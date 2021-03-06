/*
 *      Copyright (C) 2005-2018 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RemoteControlXbox.h"
#include "input/InputManager.h"
#include "input/remote/IRRemote.h"
#include "threads/SystemClock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#define XBOX_REMOTE_DEVICE_ID L"GIP:0000F50000000001"
#define XBOX_REMOTE_DEVICE_NAME "Xbox One Game Controller"

using namespace Windows::Foundation;
using namespace Windows::Media;
using namespace Windows::System;
using namespace Windows::UI::Core;

KODI::REMOTE::IRemoteControl* CRemoteControlXbox::CreateInstance()
{
  return new CRemoteControlXbox();
}

void CRemoteControlXbox::Register()
{
  CInputManager::RegisterRemoteControl(CRemoteControlXbox::CreateInstance);
}

bool CRemoteControlXbox::IsRemoteControlId(const std::wstring &deviceId)
{
  return deviceId.compare(XBOX_REMOTE_DEVICE_ID) == 0;
}

CRemoteControlXbox::CRemoteControlXbox()
  : m_button(0)
  , m_lastButton(0)
  , m_bInitialized(false)
  , m_holdTime(0)
  , m_firstClickTime(0)
  , m_lastKey(VirtualKey::None)
  , m_deviceName(XBOX_REMOTE_DEVICE_NAME)
{
}

CRemoteControlXbox::~CRemoteControlXbox()
{
  if (m_bInitialized)
    Disconnect();
}

std::string CRemoteControlXbox::GetMapFile()
{
  return "";
}

void CRemoteControlXbox::Disconnect()
{
  auto dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
  dispatcher->AcceleratorKeyActivated -= m_token;

  auto smtc = SystemMediaTransportControls::GetForCurrentView();
  if (smtc)
  {
    smtc->ButtonPressed -= m_mediatoken;
  }
  m_bInitialized = false;
}

void CRemoteControlXbox::Reset()
{
  m_button = 0;
  m_holdTime = 0;
}

void CRemoteControlXbox::Initialize()
{
  auto dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
  m_token = dispatcher->AcceleratorKeyActivated += ref new TypedEventHandler<CoreDispatcher^, AcceleratorKeyEventArgs^>
    ([this](CoreDispatcher^ sender, AcceleratorKeyEventArgs^ args) 
  {
    if (IsRemoteControlId(args->DeviceId->Data()))
      HandleAcceleratorKey(sender, args);
  });

  auto smtc = SystemMediaTransportControls::GetForCurrentView();
  if (smtc)
  {
    m_mediatoken = smtc->ButtonPressed += ref new TypedEventHandler<SystemMediaTransportControls^, SystemMediaTransportControlsButtonPressedEventArgs^>
      ([this](SystemMediaTransportControls^ sender, SystemMediaTransportControlsButtonPressedEventArgs^ args)
    {
      HandleMediaButton(args);
    });
    smtc->IsEnabled = true;
  }
  m_bInitialized = true;
}

void CRemoteControlXbox::Update()
{
  if (m_lastButton != 0)
  {
    m_button = m_lastButton;
    m_lastButton = 0;
  }
}

uint16_t CRemoteControlXbox::GetButton() const
{
  return m_button;
}

uint32_t CRemoteControlXbox::GetHoldTimeMs() const
{
  return m_holdTime;
}



void CRemoteControlXbox::HandleAcceleratorKey(CoreDispatcher^ sender, AcceleratorKeyEventArgs^ args)
{
  auto button = TranslateVirtualKey(args->VirtualKey);
  if (!button)
    return;
  
  switch (args->EventType)
  {
  case CoreAcceleratorKeyEventType::KeyDown:
  case CoreAcceleratorKeyEventType::SystemKeyDown:
  {
    if (m_lastKey != args->VirtualKey)
    {
      m_lastKey = args->VirtualKey;
      m_firstClickTime = XbmcThreads::SystemClockMillis();
      m_holdTime = 0;
    }
    else
      m_holdTime = XbmcThreads::SystemClockMillis() - m_firstClickTime;

    m_lastButton = button;
    break;
  }
  case CoreAcceleratorKeyEventType::KeyUp:
  case CoreAcceleratorKeyEventType::SystemKeyUp:
  {
    if (m_lastKey == args->VirtualKey)
      m_holdTime = XbmcThreads::SystemClockMillis() - m_firstClickTime;
    else
      m_holdTime = 0;

    m_lastKey = VirtualKey::None;
    break;
  }
  case CoreAcceleratorKeyEventType::Character:
  case CoreAcceleratorKeyEventType::SystemCharacter:
  case CoreAcceleratorKeyEventType::UnicodeCharacter:
  case CoreAcceleratorKeyEventType::DeadCharacter:
  case CoreAcceleratorKeyEventType::SystemDeadCharacter:
  default:
    break;
  }
  args->Handled = true;
}

void CRemoteControlXbox::HandleMediaButton(Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs^ args)
{
  m_lastButton = TranslateMediaKey(args->Button);
}

int32_t CRemoteControlXbox::TranslateVirtualKey(Windows::System::VirtualKey vk)
{
  switch (vk)
  {
  case VirtualKey::GamepadDPadLeft:
    return XINPUT_IR_REMOTE_LEFT;
  case VirtualKey::GamepadDPadUp:
    return XINPUT_IR_REMOTE_UP;
  case VirtualKey::GamepadDPadRight:
    return XINPUT_IR_REMOTE_RIGHT;
  case VirtualKey::GamepadDPadDown:
    return XINPUT_IR_REMOTE_DOWN;
  case VirtualKey::GamepadA:
    return XINPUT_IR_REMOTE_SELECT;
  case VirtualKey::GamepadB:
    return XINPUT_IR_REMOTE_BACK;
  case VirtualKey::GamepadX:
    return XINPUT_IR_REMOTE_CONTENTS_MENU;
  case VirtualKey::GamepadY:
    return XINPUT_IR_REMOTE_INFO;
  case VirtualKey::Clear:
    return XINPUT_IR_REMOTE_CLEAR;
  case VirtualKey::PageDown:
    return XINPUT_IR_REMOTE_CHANNEL_MINUS;
  case VirtualKey::PageUp:
    return XINPUT_IR_REMOTE_CHANNEL_PLUS;
  case VirtualKey::Number0:
    return XINPUT_IR_REMOTE_0;
  case VirtualKey::Number1:
    return XINPUT_IR_REMOTE_1;
  case VirtualKey::Number2:
    return XINPUT_IR_REMOTE_2;
  case VirtualKey::Number3:
    return XINPUT_IR_REMOTE_3;
  case VirtualKey::Number4:
    return XINPUT_IR_REMOTE_4;
  case VirtualKey::Number5:
    return XINPUT_IR_REMOTE_5;
  case VirtualKey::Number6:
    return XINPUT_IR_REMOTE_6;
  case VirtualKey::Number7:
    return XINPUT_IR_REMOTE_7;
  case VirtualKey::Number8:
    return XINPUT_IR_REMOTE_8;
  case VirtualKey::Number9:
    return XINPUT_IR_REMOTE_9;
  case VirtualKey::Decimal:
    return XINPUT_IR_REMOTE_STAR;
  case VirtualKey::GamepadView:
    return XINPUT_IR_REMOTE_DISPLAY;
  case VirtualKey::GamepadMenu:
    return XINPUT_IR_REMOTE_MENU;
  default:
    CLog::LogFunction(LOGDEBUG, __FUNCTION__, "unknown vrtual key %d", static_cast<int>(vk));
    return 0;
  }
}

int32_t CRemoteControlXbox::TranslateMediaKey(Windows::Media::SystemMediaTransportControlsButton mk)
{
  switch (mk)
  {
  case SystemMediaTransportControlsButton::ChannelDown:
    return XINPUT_IR_REMOTE_CHANNEL_MINUS;
    break;
  case SystemMediaTransportControlsButton::ChannelUp:
    return XINPUT_IR_REMOTE_CHANNEL_PLUS;
  case SystemMediaTransportControlsButton::FastForward:
    return XINPUT_IR_REMOTE_FORWARD;
  case SystemMediaTransportControlsButton::Rewind:
    return XINPUT_IR_REMOTE_REVERSE;
  case SystemMediaTransportControlsButton::Next:
    return XINPUT_IR_REMOTE_SKIP_PLUS;
  case SystemMediaTransportControlsButton::Previous:
    return XINPUT_IR_REMOTE_SKIP_MINUS;
  case SystemMediaTransportControlsButton::Pause:
    return XINPUT_IR_REMOTE_PAUSE;
  case SystemMediaTransportControlsButton::Play:
    return XINPUT_IR_REMOTE_PLAY;
  case SystemMediaTransportControlsButton::Stop:
    return XINPUT_IR_REMOTE_STOP;
  case SystemMediaTransportControlsButton::Record:
    return XINPUT_IR_REMOTE_RECORD;
  default:
    return 0;
  }
}
