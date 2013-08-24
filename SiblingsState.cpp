//---------------------------------------------------------------------------
// Copyright (C) 2013 Krzysztof Grochocki
//
// This file is part of SiblingsState
//
// SiblingsState is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// SiblingsState is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#include <IdCoderMIME.hpp>
#pragma hdrstop
#pragma argsused
#include <PluginAPI.h>
#include <inifiles.hpp>

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------

//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
//---------------------------------------------------------------------------
//Uchwyt do okna rozmowy
HWND hFrmSend;
//ID wywolania enumeracji listy kontaktow
DWORD ReplyListID = 0;
//Lista metakontaktow
TMemIniFile* SiblingsList = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
//Lista JID wraz ze stanami
TMemIniFile* ContactsStateList = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
//Lista JID wraz z indeksami konta
TMemIniFile* ContactsIndexList = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
//JID ostatnio aktywnej zakladki ze stworzonymi buttonami
UnicodeString LastActiveJID;
//Lista JIDow stworzonych buttonow
TStringList* ButtonsList = new TStringList;
//Gdy zostalo uruchomione wyladowanie wtyczki
bool UnloadExecuted = false;
//Gdy zostalo uruchomione wyladowanie wtyczki wraz z zamknieciem komunikatora
bool ForceUnloadExecuted = false;
//FORWARD-AQQ-HOOKS----------------------------------------------------------
int __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam);
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam);
int __stdcall OnListReady(WPARAM wParam, LPARAM lParam);
int __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnReplyList(WPARAM wParam, LPARAM lParam);
int __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceSiblingsStateItem0(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceSiblingsStateItem1(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceSiblingsStateItem2(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceSiblingsStateItem3(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceSiblingsStateItem4(WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Pobieranie sciezko do katalogu zawierajacego informacje o kontaktach
UnicodeString GetContactsUserDir()
{
  return StringReplace((wchar_t *)PluginLink.CallService(AQQ_FUNCTION_GETUSERDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Data\\\\Contacts\\\\";
}
//---------------------------------------------------------------------------

//Pobieranie stanu kontaktu podajac jego JID
int GetContactState(UnicodeString JID)
{
  //Pobranie stanu kontatu z listy stanow zbieranej przez wtyczke
  int State = ContactsStateList->ReadInteger("State",JID,-1);
  //Jezeli stan kontaktu nie jest zapisany
  if(State==-1)
  {
	//Pobranie domyslnej ikonki dla kontatku
	TPluginContact PluginContact;
	ZeroMemory(&PluginContact, sizeof(TPluginContact));
	PluginContact.cbSize = sizeof(TPluginContact);
	PluginContact.JID = JID.w_str();
	State = PluginLink.CallService(AQQ_FUNCTION_GETSTATEPNG_INDEX,0,(LPARAM)(&PluginContact));
  }
  //Zwrocenie ikonki stanu kontatku
  return State;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu kontaktu podajac jego JID
int GetContactIndex(UnicodeString JID)
{
  return ContactsIndexList->ReadInteger("Index",JID,0);
}
//---------------------------------------------------------------------------

//Przyjazniejsze formatowanie JID
UnicodeString FrendlyFormatJID(UnicodeString JID)
{
  //nk.pl
  if(JID.LowerCase().Pos("@nktalk.pl")) return "nk.pl";
  //Facebook
  if(JID.LowerCase().Pos("@chat.facebook.com")) return "Facebook";
  //GTalk
  if(JID.LowerCase().Pos("@public.talk.google.com")) return "GTalk";
  //Wtyczki
  if((JID.Pos("@plugin"))||(JID.Pos("@skype.plugin.aqq.eu")))
  {
	JID.Delete(JID.Pos("@"),JID.Length());
	return JID;
  }
  //Reszta
  return JID;
}
//---------------------------------------------------------------------------

//Otwieranie zakladki z metakontaktem
void OpenMetaTab(int Item)
{
  //Pobranie JID zakladki do otwarcia
  UnicodeString JID = ButtonsList->Strings[Item];
  //Otwarcie zakladki
  PluginLink.CallService(AQQ_FUNCTION_EXECUTEMSG,(WPARAM)GetContactIndex(JID),(LPARAM)JID.w_str());
}
//---------------------------------------------------------------------------

//Serwisy buttonow
int __stdcall ServiceSiblingsStateItem0(WPARAM wParam, LPARAM lParam)
{
  OpenMetaTab(0);
  return 0;
}
int __stdcall ServiceSiblingsStateItem1(WPARAM wParam, LPARAM lParam)
{
  OpenMetaTab(1);
  return 0;
}
int __stdcall ServiceSiblingsStateItem2(WPARAM wParam, LPARAM lParam)
{
  OpenMetaTab(2);
  return 0;
}
int __stdcall ServiceSiblingsStateItem3(WPARAM wParam, LPARAM lParam)
{
  OpenMetaTab(3);
  return 0;
}
int __stdcall ServiceSiblingsStateItem4(WPARAM wParam, LPARAM lParam)
{
  OpenMetaTab(4);
  return 0;
}
//---------------------------------------------------------------------------

//Hook na aktwyna zakladke lub okno rozmowy
int __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Poprzednio aktywna zakladka miala stworzone buttony
	if(!LastActiveJID.IsEmpty())
	{
	  //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(LastActiveJID,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  if(SiblingsCount>5) SiblingsCount = 5;
	  //Petla usuwania buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_DESTROYBUTTON,0,(LPARAM)(&SiblingsStateButton));
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
    }
	//Pobieranie danych nt. kontaktu
	TPluginContact ActiveTabContact = *(PPluginContact)lParam;
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)ActiveTabContact.JID;
	//Kontakt ma jakies inne metakontakty
	if(SiblingsList->SectionExists(JID))
	{
	  //Zapamietanie JID zakladki z buttonami
	  LastActiveJID = JID;
	  //Usuniecie listy stworzonych buttonow
	  ButtonsList->Clear();
	  //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(JID,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  if(SiblingsCount>5) SiblingsCount = 5;
	  //Petla tworzenia buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		UnicodeString Hint = FrendlyFormatJID(Siblings->Strings[Count]);
		SiblingsStateButton.Hint = Hint.w_str();
		SiblingsStateButton.IconIndex = GetContactState(Siblings->Strings[Count]);
		UnicodeString pszService = "sSiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszService = pszService.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_CREATEBUTTON,0,(LPARAM)(&SiblingsStateButton));
		ButtonsList->Add(Siblings->Strings[Count]);
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
	}
	//Skasowanie JID zakladki z buttonami
	else LastActiveJID = "";
	//Zapisywanie stanu kontaktu
	if(!ActiveTabContact.IsChat)
	{
	  int State = PluginLink.CallService(AQQ_FUNCTION_GETSTATEPNG_INDEX,0,(LPARAM)&ActiveTabContact);
	  ContactsStateList->WriteInteger("State",JID,State);
	}
	//Zapisywanie indeksu kontaktu
	if(!ActiveTabContact.IsChat) ContactsIndexList->WriteInteger("Index",JID,ActiveTabContact.UserIdx);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na wylaczenie komunikatora poprzez usera
int __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam)
{
  //Info o rozpoczeciu procedury zamykania komunikatora
  ForceUnloadExecuted = true;

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie okna rozmowy lub zakladki
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Pobieranie danych dt. kontaktu
	TPluginContact CloseTabContact = *(PPluginContact)lParam;
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)CloseTabContact.JID;
	//Zamknieta zakladka miala stworzone buttony
	if(JID==LastActiveJID)
	{
      //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(JID,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  if(SiblingsCount>5) SiblingsCount = 5;
	  //Petla usuwania buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_DESTROYBUTTON,0,(LPARAM)(&SiblingsStateButton));
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
	}
	//Zapisywanie stanu kontaktu
	if(!CloseTabContact.IsChat)
	{
	  int State = PluginLink.CallService(AQQ_FUNCTION_GETSTATEPNG_INDEX,0,(LPARAM)&CloseTabContact);
	  ContactsStateList->WriteInteger("State",JID,State);
	}
	//Zapisywanie indeksu kontaktu
	if(!CloseTabContact.IsChat) ContactsIndexList->WriteInteger("Index",JID,CloseTabContact.UserIdx);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê stanu kontaktu
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Pobieranie danych dt. kontaktu
	TPluginContact ContactsUpdateContact = *(PPluginContact)wParam;
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)ContactsUpdateContact.JID;
	//Zapisywanie stanu kontaktu
	if(!ContactsUpdateContact.IsChat)
	{
	  int State = PluginLink.CallService(AQQ_FUNCTION_GETSTATEPNG_INDEX,0,(LPARAM)&ContactsUpdateContact);
	  ContactsStateList->WriteInteger("State",JID,State);
	}
	//Zapisywanie indeksu kontaktu
	if(!ContactsUpdateContact.IsChat) ContactsIndexList->WriteInteger("Index",JID,ContactsUpdateContact.UserIdx);
	//Stworzono button ze wskazanym kontaktem
	if((!LastActiveJID.IsEmpty())&&(ButtonsList->IndexOf(JID)!=-1))
	{
	  //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(LastActiveJID,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  if(SiblingsCount>5) SiblingsCount = 5;
	  //Petla usuwania buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_DESTROYBUTTON,0,(LPARAM)(&SiblingsStateButton));
	  }
	  //Petla tworzenia buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		UnicodeString Hint = FrendlyFormatJID(Siblings->Strings[Count]);
		SiblingsStateButton.Hint = Hint.w_str();
		SiblingsStateButton.IconIndex = GetContactState(Siblings->Strings[Count]);
		UnicodeString pszService = "sSiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszService = pszService.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_CREATEBUTTON,0,(LPARAM)(&SiblingsStateButton));
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
    }
  }

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnListReady(WPARAM wParam, LPARAM lParam)
{
  //Pobranie ID dla enumeracji kontaktów
  ReplyListID = GetTickCount();
  //Wywolanie enumeracji kontaktow
  PluginLink.CallService(AQQ_CONTACTS_REQUESTLIST,(WPARAM)ReplyListID,0);

  return 0;
}
//---------------------------------------------------------------------------

//Hook na aktywna zakladke
int __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Jezeli nie zostala wywolana proceduta wyladowania wtyczki
	if(!UnloadExecuted)
	{
	  //Uchwyt do okna rozmowy nie zostal jeszcze pobrany
	  if(!hFrmSend)
	  {
		//Przypisanie uchwytu okna rozmowy
		hFrmSend = (HWND)(int)wParam;
	  }
	  //Pobieranie danych nt. kontaktu
	  TPluginContact PrimaryTabContact = *(PPluginContact)lParam;
	  //Pobieranie identyfikatora kontatku
	  UnicodeString JID = (wchar_t*)PrimaryTabContact.JID;
	  //Kontakt ma jakies inne metakontakty
	  if(SiblingsList->SectionExists(JID))
	  {
		//Zapamietanie JID zakladki z buttonami
		LastActiveJID = JID;
		//Usuniecie listy stworzonych buttonow
		ButtonsList->Clear();
		//Odczyt metakontaktow
		TStringList *Siblings = new TStringList;
		SiblingsList->ReadSection(JID,Siblings);
		//Pobieranie ilosci metakontaktow
		int SiblingsCount = Siblings->Count;
		if(SiblingsCount>5) SiblingsCount = 5;
		//Petla tworzenia buttonow
		for(int Count=0;Count<SiblingsCount;Count++)
		{
		  TPluginAction SiblingsStateButton;
		  ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		  SiblingsStateButton.cbSize = sizeof(TPluginAction);
		  UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		  SiblingsStateButton.pszName = pszName.w_str();
		  UnicodeString Hint = FrendlyFormatJID(Siblings->Strings[Count]);
		  SiblingsStateButton.Hint = Hint.w_str();
		  SiblingsStateButton.IconIndex = GetContactState(Siblings->Strings[Count]);
		  UnicodeString pszService = "sSiblingsStateItem"+IntToStr(Count);
		  SiblingsStateButton.pszService = pszService.w_str();
		  SiblingsStateButton.Handle = (int)hFrmSend;
		  PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_CREATEBUTTON,0,(LPARAM)(&SiblingsStateButton));
		  ButtonsList->Add(Siblings->Strings[Count]);
		}
		//Usuniecie listy metakontaktow
		delete Siblings;
	  }
	  //Skasowanie JID zakladki z buttonami
	  else LastActiveJID = "";
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na enumeracje listy kontatkow
int __stdcall OnReplyList(WPARAM wParam, LPARAM lParam)
{
  //Sprawdzanie ID wywolania enumeracji
  if((wParam==ReplyListID)&&(!ForceUnloadExecuted))
  {
    //Pobieranie danych nt. kontaktu
	TPluginContact ReplyListContact = *(PPluginContact)lParam;
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)ReplyListContact.JID;
	//Odczyt pliku INI kontaktu
	TIniFile *Ini = new TIniFile(GetContactsUserDir()+JID+".ini");
	//Odczyt informacji o metakontakcie
	TIdDecoderMIME* IdDecoderMIME = new TIdDecoderMIME();
	UnicodeString MetaParent = IdDecoderMIME->DecodeString(Ini->ReadString("Buddy", "MetaParent", ""));
	MetaParent = MetaParent.Trim();
	delete IdDecoderMIME;
	//Zamkniecie pliku INI kontaktu
	delete Ini;
	//Kontakt jest metakontaktem
	if(!MetaParent.IsEmpty())
	{
	  //Zapis podstawowej zaleznosci
	  SiblingsList->WriteBool(JID,MetaParent,true);
	  SiblingsList->WriteBool(MetaParent,JID,true);
	  //Zapis rozszerzonej zaleznosci
	  //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(MetaParent,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  //Petla usuwania buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		if(Siblings->Strings[Count]!=JID)
		{
		  SiblingsList->WriteBool(Siblings->Strings[Count],JID,true);
		  SiblingsList->WriteBool(JID,Siblings->Strings[Count],true);
		}
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
	}
	//Zapisywanie stanu kontaktu
	if(!ReplyListContact.IsChat)
	{
	  int State = PluginLink.CallService(AQQ_FUNCTION_GETSTATEPNG_INDEX,0,(LPARAM)&ReplyListContact);
	  ContactsStateList->WriteInteger("State",JID,State);
	}
	//Zapisywanie indeksu kontaktu
	if(!ReplyListContact.IsChat) ContactsIndexList->WriteInteger("Index",JID,ReplyListContact.UserIdx);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie/otwarcie okien
int __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
  //Pobranie informacji o oknie i eventcie
  TPluginWindowEvent WindowEvent = *(PPluginWindowEvent)lParam;
  int Event = WindowEvent.WindowEvent;
  UnicodeString ClassName = (wchar_t*)WindowEvent.ClassName;
  //Zamkniecie wizytowki kontaktu
  if((ClassName=="TfrmVCard")&&(Event==WINDOW_EVENT_CLOSE))
  {
	//Aktywna zakladka ma stworzone buttony
	if(!LastActiveJID.IsEmpty())
	{
	  //Odczyt metakontaktow
	  TStringList *Siblings = new TStringList;
	  SiblingsList->ReadSection(LastActiveJID,Siblings);
	  //Pobieranie ilosci metakontaktow
	  int SiblingsCount = Siblings->Count;
	  if(SiblingsCount>5) SiblingsCount = 5;
	  //Petla usuwania buttonow
	  for(int Count=0;Count<SiblingsCount;Count++)
	  {
		TPluginAction SiblingsStateButton;
		ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
		SiblingsStateButton.cbSize = sizeof(TPluginAction);
		UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
		SiblingsStateButton.pszName = pszName.w_str();
		SiblingsStateButton.Handle = (int)hFrmSend;
		PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_DESTROYBUTTON,0,(LPARAM)(&SiblingsStateButton));
	  }
	  //Usuniecie listy metakontaktow
	  delete Siblings;
	}
	//Resetowanie listy metakontaktow
	SiblingsList->Clear();
	//Pobranie ID dla enumeracji kontaktów
	ReplyListID = GetTickCount();
	//Wywolanie enumeracji kontaktow
	PluginLink.CallService(AQQ_CONTACTS_REQUESTLIST,(WPARAM)ReplyListID,0);
	//Aktywna zakladka miala stworzone buttony
	if(!LastActiveJID.IsEmpty())
	{
	  //Hook na pobieranie aktywnych zakladek
	  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_PRIMARYTAB,OnPrimaryTab);
	  //Pobieranie aktywnych zakladek
	  PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
	  //Usuniecie hooka na pobieranie aktywnych zakladek
	  PluginLink.UnhookEvent(OnPrimaryTab);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Tworzenie serwisow dla przyciskow
  PluginLink.CreateServiceFunction(L"sSiblingsStateItem0",ServiceSiblingsStateItem0);
  PluginLink.CreateServiceFunction(L"sSiblingsStateItem1",ServiceSiblingsStateItem1);
  PluginLink.CreateServiceFunction(L"sSiblingsStateItem2",ServiceSiblingsStateItem2);
  PluginLink.CreateServiceFunction(L"sSiblingsStateItem3",ServiceSiblingsStateItem3);
  PluginLink.CreateServiceFunction(L"sSiblingsStateItem4",ServiceSiblingsStateItem4);
  //Hook na aktwyna zakladke lub okno rozmowy
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_ACTIVETAB,OnActiveTab);
  //Hook na wylaczenie komunikatora poprzez usera
  PluginLink.HookEvent(AQQ_SYSTEM_BEFOREUNLOAD,OnBeforeUnload);
  //Hook na zamkniecie okna rozmowy lub zakladki
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_CLOSETAB,OnCloseTab);
  //Hook na zmianê stanu kontaktu
  PluginLink.HookEvent(AQQ_CONTACTS_UPDATE,OnContactsUpdate);
  //Hook na zakonczenie ladowania listy kontaktow przy starcie AQQ
  PluginLink.HookEvent(AQQ_CONTACTS_LISTREADY,OnListReady);
  //Hook na enumeracje listy kontatkow
  PluginLink.HookEvent(AQQ_CONTACTS_REPLYLIST,OnReplyList);
  //Hook na zamkniecie/otwarcie okien
  PluginLink.HookEvent(AQQ_SYSTEM_WINDOWEVENT,OnWindowEvent);
  //Wszystkie moduly zostaly zaladowane
  if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
  {
    //Pobranie ID dla enumeracji kontaktów
	ReplyListID = GetTickCount();
	//Wywolanie enumeracji kontaktow
	PluginLink.CallService(AQQ_CONTACTS_REQUESTLIST,(WPARAM)ReplyListID,0);
	//Hook na pobieranie aktywnych zakladek
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_PRIMARYTAB,OnPrimaryTab);
	//Pobieranie aktywnych zakladek
	PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
	//Usuniecie hooka na pobieranie aktywnych zakladek
	PluginLink.UnhookEvent(OnPrimaryTab);
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Unload()
{
  //Info o rozpoczeciu procedury wyladowania
  UnloadExecuted = true;
  //Aktywna zakladka ma stworzone buttony
  if(!LastActiveJID.IsEmpty())
  {
	//Odczyt metakontaktow
	TStringList *Siblings = new TStringList;
	SiblingsList->ReadSection(LastActiveJID,Siblings);
	//Pobieranie ilosci metakontaktow
	int SiblingsCount = Siblings->Count;
	if(SiblingsCount>5) SiblingsCount = 5;
	//Petla usuwania buttonow
	for(int Count=0;Count<SiblingsCount;Count++)
	{
	  TPluginAction SiblingsStateButton;
	  ZeroMemory(&SiblingsStateButton,sizeof(TPluginAction));
	  SiblingsStateButton.cbSize = sizeof(TPluginAction);
	  UnicodeString pszName = "SiblingsStateItem"+IntToStr(Count);
	  SiblingsStateButton.pszName = pszName.w_str();
	  SiblingsStateButton.Handle = (int)hFrmSend;
	  PluginLink.CallService(AQQ_CONTROLS_TOOLBAR "tbMain" AQQ_CONTROLS_DESTROYBUTTON,0,(LPARAM)(&SiblingsStateButton));
	}
	//Usuniecie listy metakontaktow
	delete Siblings;
  }
  //Usuwanie serwisow
  PluginLink.DestroyServiceFunction(ServiceSiblingsStateItem0);
  PluginLink.DestroyServiceFunction(ServiceSiblingsStateItem1);
  PluginLink.DestroyServiceFunction(ServiceSiblingsStateItem2);
  PluginLink.DestroyServiceFunction(ServiceSiblingsStateItem3);
  PluginLink.DestroyServiceFunction(ServiceSiblingsStateItem4);
  //Wyladowanie wszystkich hookow
  PluginLink.UnhookEvent(OnActiveTab);
  PluginLink.UnhookEvent(OnBeforeUnload);
  PluginLink.UnhookEvent(OnCloseTab);
  PluginLink.UnhookEvent(OnContactsUpdate);
  PluginLink.UnhookEvent(OnListReady);
  PluginLink.UnhookEvent(OnReplyList);
  PluginLink.UnhookEvent(OnWindowEvent);
  //Info o zakonczeniu procedury wyladowania
  UnloadExecuted = false;

  return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"SiblingsState";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.Description = L"Pokazywanie stanu metakontaktów na pasku narzêdzi w oknie rozmowy.";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";
  PluginInfo.Flag = 0;
  PluginInfo.ReplaceDefaultModule = 0;

  return &PluginInfo;
}
//---------------------------------------------------------------------------
