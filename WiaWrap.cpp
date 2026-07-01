/*++
*
Copyright (c) Microsoft Corporation. All rights reserved.

--*/
#include "StdAfx.h"
#include "DataCallback.h"
#include "resource.h"

namespace WiaWrap
{

	//////////////////////////////////////////////////////////////////////////
	//
	// namespace WiaWrap
	//

#ifndef MAX_GUID_STRING_LEN
#define MAX_GUID_STRING_LEN 39
#endif //MAX_GUID_STRING_LEN

//////////////////////////////////////////////////////////////////////////
//
// ReadPropertyLong
//

	HRESULT ReadPropertyLong(
		IWiaPropertyStorage* pWiaPropertyStorage,
		const PROPSPEC* pPropSpec,
		LONG* plResult
	)
	{
		PROPVARIANT PropVariant;

		HRESULT hr = pWiaPropertyStorage->ReadMultiple(
			1,
			pPropSpec,
			&PropVariant
		);

		// Generally, the return value should be checked against S_FALSE.
		// If ReadMultiple returns S_FALSE, it means the property name or ID
		// had valid syntax, but it didn't exist in this property set, so
		// no properties were retrieved, and each PROPVARIANT structure is set 
		// to VT_EMPTY. But the following switch statement will handle this case
		// and return E_FAIL. So the caller of ReadPropertyLong does not need
		// to check for S_FALSE explicitly.

		if (SUCCEEDED(hr))
		{
			switch (PropVariant.vt)
			{
			case VT_I1:
			{
				*plResult = (LONG)PropVariant.cVal;

				hr = S_OK;

				break;
			}

			case VT_UI1:
			{
				*plResult = (LONG)PropVariant.bVal;

				hr = S_OK;

				break;
			}

			case VT_I2:
			{
				*plResult = (LONG)PropVariant.iVal;

				hr = S_OK;

				break;
			}

			case VT_UI2:
			{
				*plResult = (LONG)PropVariant.uiVal;

				hr = S_OK;

				break;
			}

			case VT_I4:
			{
				*plResult = (LONG)PropVariant.lVal;

				hr = S_OK;

				break;
			}

			case VT_UI4:
			{
				*plResult = (LONG)PropVariant.ulVal;

				hr = S_OK;

				break;
			}

			case VT_INT:
			{
				*plResult = (LONG)PropVariant.intVal;

				hr = S_OK;

				break;
			}

			case VT_UINT:
			{
				*plResult = (LONG)PropVariant.uintVal;

				hr = S_OK;

				break;
			}

			case VT_R4:
			{
				*plResult = (LONG)(PropVariant.fltVal + 0.5);

				hr = S_OK;

				break;
			}

			case VT_R8:
			{
				*plResult = (LONG)(PropVariant.dblVal + 0.5);

				hr = S_OK;

				break;
			}

			default:
			{
				hr = E_FAIL;

				break;
			}
			}
		}

		PropVariantClear(&PropVariant);

		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// ReadPropertyGuid
	//

	HRESULT ReadPropertyGuid(
		IWiaPropertyStorage* pWiaPropertyStorage,
		const PROPSPEC* pPropSpec,
		GUID* pguidResult
	)
	{
		PROPVARIANT PropVariant;

		HRESULT hr = pWiaPropertyStorage->ReadMultiple(
			1,
			pPropSpec,
			&PropVariant
		);

		// Generally, the return value should be checked against S_FALSE.
		// If ReadMultiple returns S_FALSE, it means the property name or ID
		// had valid syntax, but it didn't exist in this property set, so
		// no properties were retrieved, and each PROPVARIANT structure is set 
		// to VT_EMPTY. But the following switch statement will handle this case
		// and return E_FAIL. So the caller of ReadPropertyGuid does not need
		// to check for S_FALSE explicitly.

		if (SUCCEEDED(hr))
		{
			switch (PropVariant.vt)
			{
			case VT_CLSID:
			{
				*pguidResult = *PropVariant.puuid;

				hr = S_OK;

				break;
			}

			case VT_BSTR:
			{
				hr = CLSIDFromString(PropVariant.bstrVal, pguidResult);

				break;
			}

			case VT_LPWSTR:
			{
				hr = CLSIDFromString(PropVariant.pwszVal, pguidResult);

				break;
			}

			case VT_LPSTR:
			{
				WCHAR wszGuid[MAX_GUID_STRING_LEN];
				mbstowcs_s(NULL, wszGuid, MAX_GUID_STRING_LEN, PropVariant.pszVal, MAX_GUID_STRING_LEN);

				wszGuid[MAX_GUID_STRING_LEN - 1] = L'\0';

				hr = CLSIDFromString(wszGuid, pguidResult);

				break;
			}

			default:
			{
				hr = E_FAIL;

				break;
			}
			}
		}

		PropVariantClear(&PropVariant);

		return hr;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// WiaGetNumDevices
	//

	HRESULT	WiaGetNumDevices(
			IWiaDevMgr* pSuppliedWiaDevMgr,
			ULONG* pulNumDevices
		)
	{
		HRESULT hr;

		// Validate and initialize output parameters

		if (pulNumDevices == NULL)
		{
			return E_POINTER;
		}

		*pulNumDevices = 0;

		// Create a connection to the local WIA device manager

		CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

		if (pWiaDevMgr == NULL)
		{
			hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

			if (FAILED(hr))
			{
				return hr;
			}
		}

		// Get a list of all the WIA devices on the system

		CComPtr<IEnumWIA_DEV_INFO> pIEnumWIA_DEV_INFO;

		hr = pWiaDevMgr->EnumDeviceInfo(
			0,
			&pIEnumWIA_DEV_INFO
		);

		if (FAILED(hr))
		{
			return hr;
		}

		// Get the number of WIA devices

		ULONG celt;

		hr = pIEnumWIA_DEV_INFO->GetCount(&celt);

		if (FAILED(hr))
		{
			return hr;
		}

		*pulNumDevices = celt;

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// DefaultProgressCallback
	//

	HRESULT CALLBACK DefaultProgressCallback(
		LONG   lStatus,
		LONG   lPercentComplete,
		PVOID  pParam
	)
	{
		CProgressDlg* pProgressDlg = (CProgressDlg*)pParam;

		if (pProgressDlg == NULL)
		{
			return E_POINTER;
		}

		// If the user has pressed the cancel button, abort transfer

		if (pProgressDlg->Cancelled())
		{
			return S_FALSE;
		}

		// Form the message text

		UINT uID;

		switch (lStatus)
		{
		case IT_STATUS_TRANSFER_FROM_DEVICE:
		{
			uID = IDS_STATUS_TRANSFER_FROM_DEVICE;
			break;
		}

		case IT_STATUS_PROCESSING_DATA:
		{
			uID = IDS_STATUS_PROCESSING_DATA;
			break;
		}

		case IT_STATUS_TRANSFER_TO_CLIENT:
		{
			uID = IDS_STATUS_TRANSFER_TO_CLIENT;
			break;
		}

		default:
		{
			return E_INVALIDARG;
		}
		}

		TCHAR szFormat[DEFAULT_STRING_SIZE] = _T("%d");

		// rasa LoadString(g_hInstance, uID, szFormat, COUNTOF(szFormat));
		LoadString(GetModuleHandle(NULL), uID, szFormat, COUNTOF(szFormat));


		TCHAR szStatusText[DEFAULT_STRING_SIZE];

		_sntprintf_s(szStatusText, COUNTOF(szStatusText), COUNTOF(szStatusText) - 1, szFormat, lPercentComplete);

		szStatusText[COUNTOF(szStatusText) - 1] = _T('\0');

		// Update the progress bar values

		pProgressDlg->SetMessage(szStatusText);

		pProgressDlg->SetPercent(lPercentComplete);

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// WiaGetImage
	//

	HRESULT WiaGetImage(
		HWND                 hWndParent,
		LONG                 lDeviceType,
		LONG                 lFlags,
		LONG                 lIntent,
		IWiaDevMgr* pSuppliedWiaDevMgr,
		IWiaItem* pSuppliedItemRoot,
		PFNPROGRESSCALLBACK  pfnProgressCallback,
		PVOID                pProgressCallbackParam,
		GUID* pguidFormat,
		LONG* plCount,
		IStream*** pppStream
	)
	{
		HRESULT hr;

		// Validate and initialize output parameters

		if (plCount == NULL)
		{
			return E_POINTER;
		}

		if (pppStream == NULL)
		{
			return E_POINTER;
		}

		*plCount = 0;
		*pppStream = NULL;

		// Initialize the local root item variable with the supplied value.
		// If no value is supplied, display the device selection common dialog.

		CComPtr<IWiaItem> pItemRoot = pSuppliedItemRoot;

		if (pItemRoot == NULL)
		{
			// Initialize the device manager pointer with the supplied value
			// If no value is supplied, connect to the local device manager

			CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

			if (pWiaDevMgr == NULL)
			{
				hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

				if (FAILED(hr))
				{
					return hr;
				}
			}

			// Display the device selection common dialog

			hr = pWiaDevMgr->SelectDeviceDlg(
				hWndParent,
				lDeviceType,
				lFlags,
				0,
				&pItemRoot
			);

			if (FAILED(hr) || hr == S_FALSE)
			{
				return hr;
			}
		}

		// Display the image selection common dialog 

		CComPtrArray<IWiaItem> ppIWiaItem;

		hr = pItemRoot->DeviceDlg(
			hWndParent,
			lFlags,
			lIntent,
			&ppIWiaItem.Count(),
			&ppIWiaItem
		);

		if (FAILED(hr) || hr == S_FALSE)
		{
			return hr;
		}

		// For ADF scanners, the common dialog explicitly sets the page count to one.
		// So in order to transfer multiple images, set the page count to ALL_PAGES
		// if the WIA_DEVICE_DIALOG_SINGLE_IMAGE flag is not specified, 

		if (!(lFlags & WIA_DEVICE_DIALOG_SINGLE_IMAGE))
		{
			// Get the property storage interface pointer for the root item

			CComQIPtr<IWiaPropertyStorage> pWiaRootPropertyStorage(pItemRoot);

			if (pWiaRootPropertyStorage == NULL)
			{
				return E_NOINTERFACE;
			}

			// Determine if the selected device is a scanner or not

			PROPSPEC specDevType;

			specDevType.ulKind = PRSPEC_PROPID;
			specDevType.propid = WIA_DIP_DEV_TYPE;

			LONG nDevType;

			hr = ReadPropertyLong(
				pWiaRootPropertyStorage,
				&specDevType,
				&nDevType
			);

			if (SUCCEEDED(hr) && (GET_STIDEVICE_TYPE(nDevType) == StiDeviceTypeScanner))
			{
				// Determine if the document feeder is selected or not

				PROPSPEC specDocumentHandlingSelect;

				specDocumentHandlingSelect.ulKind = PRSPEC_PROPID;
				specDocumentHandlingSelect.propid = WIA_DPS_DOCUMENT_HANDLING_SELECT;

				LONG nDocumentHandlingSelect;

				hr = ReadPropertyLong(
					pWiaRootPropertyStorage,
					&specDocumentHandlingSelect,
					&nDocumentHandlingSelect
				);

				if (SUCCEEDED(hr) && (nDocumentHandlingSelect & FEEDER))
				{
					PROPSPEC specPages;

					specPages.ulKind = PRSPEC_PROPID;
					specPages.propid = WIA_DPS_PAGES;

					PROPVARIANT varPages;

					varPages.vt = VT_I4;
					varPages.lVal = ALL_PAGES;

					pWiaRootPropertyStorage->WriteMultiple(
						1,
						&specPages,
						&varPages,
						WIA_DPS_FIRST
					);

					PropVariantClear(&varPages);
				}
			}
		}

		// If a status callback function is not supplied, use the default.
		// The default displays a simple dialog with a progress bar and cancel button.

		CComPtr<CProgressDlg> pProgressDlg;

		if (pfnProgressCallback == NULL)
		{
			pfnProgressCallback = DefaultProgressCallback;

			pProgressDlg = new CProgressDlg(hWndParent);

			pProgressCallbackParam = (CProgressDlg*)pProgressDlg;
		}

		// Create the data callback interface

		CComPtr<CDataCallback> pDataCallback = new CDataCallback(
			pfnProgressCallback,
			pProgressCallbackParam,
			plCount,
			pppStream
		);

		if (pDataCallback == NULL)
		{
			return E_OUTOFMEMORY;
		}

		// Start the transfer of the selected items

		for (int i = 0; i < ppIWiaItem.Count(); ++i)
		{
			// Get the interface pointers

			CComQIPtr<IWiaPropertyStorage> pWiaPropertyStorage(ppIWiaItem[i]);

			if (pWiaPropertyStorage == NULL)
			{
				return E_NOINTERFACE;
			}

			CComQIPtr<IWiaDataTransfer> pIWiaDataTransfer(ppIWiaItem[i]);

			if (pIWiaDataTransfer == NULL)
			{
				return E_NOINTERFACE;
			}

			// Set the transfer type

			PROPSPEC specTymed;

			specTymed.ulKind = PRSPEC_PROPID;
			specTymed.propid = WIA_IPA_TYMED;

			PROPVARIANT varTymed;

			varTymed.vt = VT_I4;
			varTymed.lVal = TYMED_CALLBACK;

			hr = pWiaPropertyStorage->WriteMultiple(
				1,
				&specTymed,
				&varTymed,
				WIA_IPA_FIRST
			);

			PropVariantClear(&varTymed);

			if (FAILED(hr))
			{
				return hr;
			}

			// If there is no transfer format specified, use the device default

			GUID guidFormat = GUID_NULL;

			if (pguidFormat == NULL)
			{
				pguidFormat = &guidFormat;
			}

			if (*pguidFormat == GUID_NULL)
			{
				PROPSPEC specPreferredFormat;

				specPreferredFormat.ulKind = PRSPEC_PROPID;
				specPreferredFormat.propid = WIA_IPA_PREFERRED_FORMAT;

				hr = ReadPropertyGuid(
					pWiaPropertyStorage,
					&specPreferredFormat,
					pguidFormat
				);

				if (FAILED(hr))
				{
					return hr;
				}
			}

			// Set the transfer format

			PROPSPEC specFormat;

			specFormat.ulKind = PRSPEC_PROPID;
			specFormat.propid = WIA_IPA_FORMAT;

			PROPVARIANT varFormat;

			varFormat.vt = VT_CLSID;
			varFormat.puuid = (CLSID*)CoTaskMemAlloc(sizeof(CLSID));

			if (varFormat.puuid == NULL)
			{
				return E_OUTOFMEMORY;
			}

			*varFormat.puuid = *pguidFormat;

			hr = pWiaPropertyStorage->WriteMultiple(
				1,
				&specFormat,
				&varFormat,
				WIA_IPA_FIRST
			);

			PropVariantClear(&varFormat);

			if (FAILED(hr))
			{
				return hr;
			}

			// Read the transfer buffer size from the device, default to 64K

			PROPSPEC specBufferSize;

			specBufferSize.ulKind = PRSPEC_PROPID;
			specBufferSize.propid = WIA_IPA_BUFFER_SIZE;

			LONG nBufferSize;

			hr = ReadPropertyLong(
				pWiaPropertyStorage,
				&specBufferSize,
				&nBufferSize
			);

			if (FAILED(hr))
			{
				nBufferSize = 64 * 1024;
			}

			// Choose double buffered transfer for better performance

			WIA_DATA_TRANSFER_INFO WiaDataTransferInfo = { 0 };

			WiaDataTransferInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
			WiaDataTransferInfo.ulBufferSize = 2 * nBufferSize;
			WiaDataTransferInfo.bDoubleBuffer = TRUE;

			// Start the transfer

			hr = pIWiaDataTransfer->idtGetBandedData(
				&WiaDataTransferInfo,
				pDataCallback
			);

			if (FAILED(hr) || hr == S_FALSE)
			{
				return hr;
			}
		}

		return S_OK;
	}

	HRESULT WiaGetEvents(
		LONG					lDeviceType,
		LONG					lFlags,
		LONG					lIntent,
		IWiaDevMgr* pSuppliedWiaDevMgr,
		IWiaItem* pSuppliedItemRoot,
		PFNPROGRESSCALLBACK		pfnProgressCallback,
		PVOID					pProgressCallbackParam,
		GUID* pguidFormat,
		LONG* plCount,
		IStream*** pppStream
	)
	{
		HRESULT hr;

		// Validate and initialize output parameters

		if (plCount == NULL)
		{
			return E_POINTER;
		}

		if (pppStream == NULL)
		{
			return E_POINTER;
		}

		*plCount = 0;
		*pppStream = NULL;

		// Initialize the local root item variable with the supplied value.
		// If no value is supplied, display the device selection common dialog.

		CComPtr<IWiaItem> pItemRoot = pSuppliedItemRoot;

		if (pItemRoot == NULL)
		{
			LOG_SAVE << "pItemRoot is NULL";

			// Initialize the device manager pointer with the supplied value
			// If no value is supplied, connect to the local device manager

			CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

			if (pWiaDevMgr == NULL)
			{
				hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

				if (FAILED(hr))
				{
					return hr;
				}
			}

			// Display the device selection common dialog

			IWiaDevMgr* RooWiaDevMgr;

			// Declare PROPSPECs and PROPVARIANTs, and initialize them to zero.
			PROPSPEC PropSpec[1] = { 0 };
			PROPVARIANT PropVar[1] = { 0 };

			// How many properties are we querying for?
			const ULONG c_nPropertyCount = sizeof(PropSpec) / sizeof(PropSpec[0]);

			// Define which properties we want to read. We want the
			// device ID, so we can create the device.
			PropSpec[0].ulKind = PRSPEC_PROPID;
			PropSpec[0].propid = WIA_DIP_DEV_ID;


			// Get the property storage interface pointer for the root item

			CComQIPtr<IWiaPropertyStorage> pWiaRootPropertyStorage(pItemRoot);

			//if (pWiaRootPropertyStorage == NULL)
			//{
			//    return E_NOINTERFACE;
			//}

			// Determine if the selected device is a scanner or not

			PROPSPEC specDevType;

			specDevType.ulKind = PRSPEC_PROPID;
			specDevType.propid = WIA_DIP_DEV_TYPE;

			LONG nDevType;

			hr = ReadPropertyLong(
				pWiaRootPropertyStorage,
				&specDevType,
				&nDevType
			);

			if (SUCCEEDED(hr) && (GET_STIDEVICE_TYPE(nDevType) == StiDeviceTypeScanner))
			{
				// Determine if the document feeder is selected or not

				PROPSPEC specDocumentHandlingSelect;

				specDocumentHandlingSelect.ulKind = PRSPEC_PROPID;
				specDocumentHandlingSelect.propid = WIA_DPS_DOCUMENT_HANDLING_SELECT;

				LONG nDocumentHandlingSelect;

				hr = ReadPropertyLong(
					pWiaRootPropertyStorage,
					&specDocumentHandlingSelect,
					&nDocumentHandlingSelect
				);

				if (SUCCEEDED(hr) && (nDocumentHandlingSelect & FEEDER))
				{
					PROPSPEC specPages;

					specPages.ulKind = PRSPEC_PROPID;
					specPages.propid = WIA_DPS_PAGES;

					PROPVARIANT varPages;

					varPages.vt = VT_I4;
					varPages.lVal = ALL_PAGES;

					pWiaRootPropertyStorage->WriteMultiple(1, &specPages, &varPages, WIA_DPS_FIRST);

					PropVariantClear(&varPages);
				}
			}

			// Ask for the property values
			HRESULT hr = pWiaRootPropertyStorage->ReadMultiple(c_nPropertyCount, PropSpec, PropVar);

// rasa			CRus1scan* WiaWrapDev = new CRus1scan(NULL, &RooWiaDevMgr);
// 02.08.24			hr = WiaWrapDev->GetFromDevice(pWiaDevMgr, PropVar[0].bstrVal, &pItemRoot);

			//rasa		hr = pWiaDevMgr->SelectDeviceDlg(hWndParent, lDeviceType, lFlags, 0, &pItemRoot);

			if (FAILED(hr) || hr == S_FALSE)
			{
				return hr;
			}
		}

		IEnumWiaItem* pEnum = NULL;
		IEnumWIA_DEV_CAPS* pEnumDevCaps;
		WIA_DEV_CAP* pDevCaps = NULL;
		ULONG lCount;
		ULONG nFetched;
		CString eventStr;
		int i;

		// Get the property storage interface pointer for the root item
		CComQIPtr<IWiaPropertyStorage> pWiaRootPropertyStorage(pItemRoot);

		// Declare PROPSPECs and PROPVARIANTs, and initialize them to zero.
		PROPSPEC PropSpec[3] = { 0 };
		PROPVARIANT PropVar[3] = { 0 };

		// How many properties are we querying for?
		const ULONG c_nPropertyCount = sizeof(PropSpec) / sizeof(PropSpec[0]);

		// Define which properties we want to read: Device ID.
		// This is what we'd use to create the device.		
		PropSpec[0].ulKind = PRSPEC_PROPID;
		PropSpec[0].propid = WIA_DIP_DEV_ID;

		// Device Name
		PropSpec[1].ulKind = PRSPEC_PROPID;
		PropSpec[1].propid = WIA_DIP_DEV_NAME;


		// Device description
		PropSpec[2].ulKind = PRSPEC_PROPID;
		PropSpec[2].propid = WIA_DIP_DEV_DESC;


		// Ask for the property values
		hr = pWiaRootPropertyStorage->ReadMultiple(c_nPropertyCount, PropSpec, PropVar);
		if (SUCCEEDED(hr))
		{
			//
			// IWiaPropertyStorage::ReadMultiple will return S_FALSE if some
			// properties could not be read, so we have to check the return
			// types for each requested item.
			//

			//
			// Check the return type for the device ID
			//
			if (VT_BSTR == PropVar[0].vt)
			{
				//
				// Do something with the device ID
				//
				LOG_SAVE << "WIA_DIP_DEV_ID:";
				LOG_SAVE << PropVar[0].bstrVal;
			}

			//
			// Check the return type for the device name
			//
			if (VT_BSTR == PropVar[1].vt)
			{
				//
				// Do something with the device name
				// LOG_SAVE << "WIA_DIP_DEV_NAME: %ws\n";
				eventStr = PropVar[1].bstrVal;
			}
			//
			// Check the return type for the device description
			//
			if (VT_BSTR == PropVar[2].vt)
			{
				//
				// Do something with the device description
				//
				LOG_SAVE << "WIA_DIP_DEV_DESC:";
				LOG_SAVE << PropVar[2].bstrVal;
			}

			//
			// Free the returned PROPVARIANTs
			//
			FreePropVariantArray(c_nPropertyCount, PropVar);
		}
		else
		{
			LOG_SAVE << "Error calling IWiaPropertyStorage::ReadMultiple";
		}

		LOG_SAVE << "pItemRoot->EnumDeviceCapabilities";
		hr = pItemRoot->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS, &pEnumDevCaps);
		if (hr == S_OK)
		{
			LOG_SAVE << "S_OK";
			hr = pEnumDevCaps->GetCount(&lCount);
			if (lCount != 0)
			{
				LOG_SAVE << "Count != 0";
				pDevCaps = new WIA_DEV_CAP[lCount];
				pEnumDevCaps->Next(lCount, pDevCaps, &nFetched);
				//         evestr = "Device Capabilities: \n";
				for (i = 0; i < nFetched; i++)
				{
					if (3 == pDevCaps[i].ulFlags)	// "3" means Button
						// pDevCaps[i].ulFlags |= WIA_DEVICE_PROPERTY_BROADCAST_CONNECT, WIA_DEVICE_FLAGS_NO_BROADCAST_CONNECT;
						// Приложение может определить, является ли событие типом действия или типом уведомления - значение ulFlags структуры WIA_DEV_CAP, возвращаемое в перечислении событий.
						// WIA_IS_DEFAULT_HANDLER	The currently registered handler should be used.This is the only valid value when enumerating event handlers.It is not a valid value when enumerating event capabilities of a device.
						// WIA_ACTION_EVENT	The event is of the action type, so programs that use persistent registration APIs, IWiaDevMgr::RegisterEventCallbackProgram and IWiaDevMgr::RegisterEventCallbackCLSID, can receive it.
						// WIA_NOTIFICATION_EVENT	The event is of the notification type, so programs that use the runtime registration function, IWiaDevMgr::RegisterEventCallbackInterface, can receive it.
					{
						eventStr = pDevCaps[i].bstrName;

						LOG_SAVE << "bstrName" << eventStr;
						//	rasa					if (mfcElements.descrText) mfcElements.descrText->SetWindowText(CString(pDevCaps[i].bstrDescription));
												// eventStr = pDevCaps[i].bstrCommandline;
												// eventStr = pDevCaps[i].bstrDescription;
												// eventStr = pDevCaps[i].bstrIcon;
						OLECHAR* guidString;
						HRESULT hR = StringFromCLSID(pDevCaps[i].guid, &guidString);
						eventStr = guidString;
						LOG_SAVE << "guidString" << eventStr;
						// ensure memory is freed
						::CoTaskMemFree(guidString);
					}
				}
			}
		}
	}	

	HRESULT WiaTakeImage(
		HWND                 hWndParent,
		LONG                 lDeviceType,
		LONG                 lFlags,
		LONG                 lIntent,
		IWiaDevMgr* pSuppliedWiaDevMgr,
		IWiaItem* pSuppliedItemRoot,
		PFNPROGRESSCALLBACK  pfnProgressCallback,
		PVOID                pProgressCallbackParam,
		GUID* pguidFormat,
		LONG* plCount,
		IStream*** pppStream
	)
	{
		HRESULT hr;

		// Validate and initialize output parameters

		if (plCount == NULL)
		{
			return E_POINTER;
		}

		if (pppStream == NULL)
		{
			return E_POINTER;
		}

		*plCount = 0;
		*pppStream = NULL;

		// Initialize the local root item variable with the supplied value.
		// If no value is supplied, display the device selection common dialog.

		CComPtr<IWiaItem> pItemRoot = pSuppliedItemRoot;

		if (pItemRoot == NULL)
		{
			// Initialize the device manager pointer with the supplied value
			// If no value is supplied, connect to the local device manager

			CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

			if (pWiaDevMgr == NULL)
			{
				hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

				if (FAILED(hr))
				{
					return hr;
				}
			}

			// Display the device selection common dialog

			hr = pWiaDevMgr->SelectDeviceDlg(
				hWndParent,
				lDeviceType,
				lFlags,
				0,
				&pItemRoot
			);

			if (FAILED(hr) || hr == S_FALSE)
			{
				return hr;
			}
		}

		// When I try to take a picture:
		GUID command;
		CString str;

		command = WIA_CMD_TAKE_PICTURE;

		IWiaItem* ppIWiaItem = NULL;

		hr = pItemRoot->DeviceCommand(
			0,
			&command,
			&ppIWiaItem
		);

		if (hr == S_OK)
		{
			str = "Remote capture successful!";
			AfxMessageBox(str);
		}


		if (FAILED(hr) || hr == S_FALSE)
		{
			str = "Error.";
			AfxMessageBox(str);
			return hr;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// WiaGetCapabilities
	HRESULT	WiaGetCapabilities(
			HWND                 hWndParent,
			LONG                 lDeviceType,
			LONG                 lFlags,
			LONG                 lIntent,
			IWiaDevMgr* pSuppliedWiaDevMgr,
			IWiaItem* pSuppliedItemRoot,
			PFNPROGRESSCALLBACK  pfnProgressCallback,
			PVOID                pProgressCallbackParam,
			GUID* pguidFormat,
			LONG* plCount,
			IStream*** pppStream
		)
	{
		HRESULT hr;

		// Validate and initialize output parameters

		if (plCount == NULL)
		{
			return E_POINTER;
		}

		if (pppStream == NULL)
		{
			return E_POINTER;
		}

		*plCount = 0;
		*pppStream = NULL;

		// Initialize the local root item variable with the supplied value.
		// If no value is supplied, display the device selection common dialog.

		CComPtr<IWiaItem> pItemRoot = pSuppliedItemRoot;

		if (pItemRoot == NULL)
		{
			// Initialize the device manager pointer with the supplied value
			// If no value is supplied, connect to the local device manager

			CComPtr<IWiaDevMgr> pWiaDevMgr = pSuppliedWiaDevMgr;

			if (pWiaDevMgr == NULL)
			{
				hr = pWiaDevMgr.CoCreateInstance(CLSID_WiaDevMgr);

				if (FAILED(hr))
				{
					return hr;
				}
			}

			// Display the device selection common dialog

			hr = pWiaDevMgr->SelectDeviceDlg(
				hWndParent,
				lDeviceType,
				lFlags,
				0,
				&pItemRoot
			);

			if (FAILED(hr) || hr == S_FALSE)
			{
				return hr;
			}
		}

		IEnumWiaItem* pEnum = NULL;
		IEnumWIA_DEV_CAPS* pEnumDevCaps;
		WIA_DEV_CAP* pDevCaps = NULL;
		ULONG lCount;
		ULONG nFetched;
		CString str;
		CString str1;
		int i;

		hr = pItemRoot->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS, &pEnumDevCaps);
		if (hr == S_OK)
		{
			hr = pEnumDevCaps->GetCount(&lCount);
			if (lCount != 0)
			{
				pDevCaps = new WIA_DEV_CAP[lCount];
				pEnumDevCaps->Next(lCount, pDevCaps, &nFetched);
				str = "Device Capabilities: \n";
				for (i = 0; i < nFetched; i++)
				{
					str1 = pDevCaps[i].bstrName;
					str += "\n";
					str += str1;
				}
				//AfxMessageBox(str);
			}
		}
	}

}; // namespace WiaWrap
