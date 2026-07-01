#include "StdAfx.h"
#include "EventCallback.h"
#include "WiaWrap.h"

#include "helper_funcs.h"

namespace WiaWrap
{
	//////////////////////////////////////////////////////////////////////////
	// CEventCallback::CEventCallback
	CEventCallback::CEventCallback()
	{
		m_cRef = 0;
		m_nNumDevices = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// CEventCallback::QueryInterface

	STDMETHODIMP CEventCallback::QueryInterface(REFIID iid, LPVOID* ppvObj)
	{
		if (ppvObj == NULL)
		{
			LOG_SAVE << "QueryInterface: ppvObj is NULL";
			return E_POINTER;
		}

		if (iid == IID_IUnknown)
		{
			LOG_SAVE << "iid == IID_IUnknown";
			LOG_SAVE << ppvObj;
			*ppvObj = (IUnknown*)this;
		}
		else if (iid == IID_IWiaEventCallback)
		{
			LOG_SAVE << "iid == IID_IWiaEventCallback";
			LOG_SAVE << ppvObj;
			*ppvObj = (IWiaEventCallback*)this;
		}
		else
		{			
			*ppvObj = NULL;
//			LOG_SAVE << "The iid is not defined";
//			LOG_SAVE << iid.Data2;
			return E_NOINTERFACE;
		}

		AddRef();
		LOG_SAVE << "QueryInterface returns OK";
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// CEventCallback::AddRef
	//

	STDMETHODIMP_(ULONG) CEventCallback::AddRef()
	{
//		LOG_SAVE << "ADD REF";
		return InterlockedIncrement(&m_cRef);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// CEventCallback::Release
	//

	STDMETHODIMP_(ULONG) CEventCallback::Release()
	{
//		LOG_SAVE << "Release";
		LONG cRef = InterlockedDecrement(&m_cRef);

		if (cRef == 0)
		{
//			LOG_SAVE << "Delete this";
			delete this;
		}

//		LOG_SAVE << "cRef returned";
		return cRef;
	}

	//////////////////////////////////////////////////////////////////////////
	// CEventCallback::ImageEventCallback
	// The IWiaEventCallback::ImageEventCallback method is invoked by the WIA
	// run-time system when a hardware device event occurs.
	STDMETHODIMP CEventCallback::ImageEventCallback(
		LPCGUID pEventGuid,
		BSTR    bstrEventDescription,
		BSTR    bstrDeviceID,
		BSTR    bstrDeviceDescription,
		DWORD   dwDeviceType,
		BSTR    bstrFullItemName,
		ULONG*  pulEventType,
		ULONG   ulReserved
	)
	{	
		LOG_SAVE << bstr_to_u32(bstrEventDescription) << " / " << pEventGuid << "on " << bstr_to_u32(bstrDeviceDescription) << " / " << bstr_to_u32(bstrDeviceID) << " was occured.";
		return WiaGetNumDevices(NULL, &m_nNumDevices);
	}

	//////////////////////////////////////////////////////////////////////////
	// CEventCallback::Register
	HRESULT CEventCallback::Register()
	{
		HRESULT hr;

		// Create a connection to the local WIA device manager

		CComPtr<IWiaDevMgr> pWiaDevMgr;

		hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

		if (FAILED(hr))
		{
			LOG_SAVE << "CoCreateInstance FAILED";
			LOG_SAVE << GetLastError();
			return hr;
		}

		// Register the callback interface
		LOG_SAVE << "Event WIA_EVENT_DEVICE_CONNECTED registered";
		hr = pWiaDevMgr->RegisterEventCallbackInterface(
			0,
			NULL,
			&WIA_EVENT_DEVICE_CONNECTED,
			this,
			&m_pConnectEventObject
		);
		if (FAILED(hr))
		{
			LOG_SAVE << "Event WIA_EVENT_DEVICE_CONNECTED registering FAILED";
			LOG_SAVE << GetLastError();
			return hr;
		}

/*		GUID guid; Êíîïêà E - mail
		CLSIDFromString(L"{4d36e96e-e325-11ce-bfc1-08002be10318}", &guid);	*/
		GUID WIA_CUSTO_EVENT = { 0xD0C45E80, 0x6B8C, 0x11D3, { 0xa8, 0x2a, 0x00, 0xA0, 0x24, 0x91, 0xdf, 0x4e } }; 
		hr = pWiaDevMgr->RegisterEventCallbackInterface(
			0,
			NULL,
			&WIA_CUSTO_EVENT,
			this,
			&m_pCustomEventObject
		);
		if (FAILED(hr))
		{
			LOG_SAVE << "Registering Custom Event FAILED!";
			LOG_SAVE << GetLastError();
			return hr;
		}
		LOG_SAVE << "Custom Event registered";

		LOG_SAVE << "Event WIA_EVENT_DEVICE_DISCONNECTED registered";
		hr = pWiaDevMgr->RegisterEventCallbackInterface(
			0,
			NULL,
			&WIA_EVENT_DEVICE_DISCONNECTED,
			this,
			&m_pDisconnectEventObject
		);

		if (FAILED(hr))
		{
			LOG_SAVE << "FAIL!";
			LOG_SAVE << GetLastError();
			return hr;
		}

		// Get the current count of all the WIA devices on the system

		hr = WiaGetNumDevices(pWiaDevMgr, &m_nNumDevices);
		if (FAILED(hr))
		{
			LOG_SAVE << "WiaGetNumDevices FAILED";
			LOG_SAVE << GetLastError();
			return hr;
		}
		LOG_SAVE << "WiaGetNumDevices OK";
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// CEventCallback::GetNumDevices
	//

	ULONG CEventCallback::GetNumDevices() const
	{
		return m_nNumDevices;
	}

}; // namespace WiaWrap
