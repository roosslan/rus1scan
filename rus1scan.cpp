#include "stdafx.h"
#include "rus1scan.h"
#include "helper_funcs.h"

Crus1scan::Crus1scan(IWiaDevMgr** device_manager) {
    LOG_SAVE << "Crus1scan Constructor";

    // Initialize COM
    // HRESULT hr = CoInitialize(NULL);
    // в InitInstance был вызван HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); */
    // if (SUCCEEDED(hr))
    {
        /* Create the device manager */
        IWiaDevMgr* pWiaDevMgr = nullptr;

        HRESULT hr = create_wia_device_manager(&pWiaDevMgr);
        if (SUCCEEDED(hr)) {
            /* Enumerate all of the WIA devices */
            hr = enumerate_wia_devices(pWiaDevMgr);
            if (FAILED(hr)) {
                LOG_SAVE << "Error calling EnumerateWiaDevices";
            }
            
            /* Release the device manager */
            pWiaDevMgr->Release();
            pWiaDevMgr = nullptr;
        }
        else {
            LOG_SAVE << "Error calling CreateWiaDeviceManager";
        }

        /* Uninitialize COM */
        CoUninitialize();
    }
}

HRESULT Crus1scan::transfer_wia_item(IWiaItem* pWiaItem) {
    LOG_SAVE << "Start transferring";

    /* Validate arguments */
    if (nullptr == pWiaItem)
        return E_INVALIDARG;

    /* Get the IWiaPropertyStorage interface so we can set required properties */
    IWiaPropertyStorage* pWiaPropertyStorage = nullptr;

    HRESULT hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, reinterpret_cast<void**>(&pWiaPropertyStorage));
    if (SUCCEEDED(hr)) {
        /* Prepare PROPSPECs and PROPVARIANTs for setting the media type and format */
        PROPSPEC prop_spec[2] = {};
        PROPVARIANT prop_variant[2] = {};
        constexpr ULONG prop_count = std::size(prop_variant);

        /* Use BMP as the output format */
        GUID guid_output_format = WiaImgFmt_BMP;

        /* Initialize the PROPSPECs */
        prop_spec[0].ulKind = PRSPEC_PROPID;
        prop_spec[0].propid = WIA_IPA_FORMAT;
        prop_spec[1].ulKind = PRSPEC_PROPID;
        prop_spec[1].propid = WIA_IPA_TYMED;

        /* Initialize the PROPVARIANTs */
        prop_variant[0].vt = VT_CLSID;
        prop_variant[0].puuid = &guid_output_format;
        prop_variant[1].vt = VT_I4;
        prop_variant[1].lVal = TYMED_FILE;


        /* Set the properties */
        hr = pWiaPropertyStorage->WriteMultiple(prop_count, prop_spec, prop_variant, WIA_IPA_FIRST);
        if (SUCCEEDED(hr))
        {
            /* Get the IWiaDataTransfer interface */
            IWiaDataTransfer* pWiaDataTransfer = nullptr;
            hr = pWiaItem->QueryInterface(IID_IWiaDataTransfer, (void**)&pWiaDataTransfer);
            if (SUCCEEDED(hr))
            {
                /* Create our callback class */
                if (auto p_callback = new CWiaDataCallback)
                {
                    /* Get the IWiaDataCallback interface from our callback class */
                    IWiaDataCallback* pWiaDataCallback = nullptr;
                    hr = p_callback->QueryInterface(IID_IWiaDataCallback, reinterpret_cast<void**>(&pWiaDataCallback));
                    if (SUCCEEDED(hr))
                    {
                        /* Perform the transfer using default settings */
                        STGMEDIUM stg_medium;
                        stg_medium = {0};

                        LOG_SAVE << "idtGetData";
                        hr = pWiaDataTransfer->idtGetData(&stg_medium, pWiaDataCallback);
                        if (S_OK == hr) {
                            /* NB: filename is WCHAR, not TCHAR */
                            LOG_SAVE << "Transferred filename: " << stg_medium.lpszFileName;

                            /* Release memory, associated with the stg_medium */
                            ReleaseStgMedium(&stg_medium);
                        }
                        else {
                            LOG_SAVE << "pWiaDataTransfer->idtGetData failed";
                        }

                        /* Release the callback interface */
                        pWiaDataCallback->Release();
                        pWiaDataCallback = nullptr;
                    }
                    else {
                        LOG_SAVE << "pCallback->QueryInterface failed on IID_IWiaDataCallback";
                    }

                    /* Release our callback. It should now delete itself */
                    p_callback->Release();
                    p_callback = nullptr;
                }
                else {
                    LOG_SAVE << "Unable to create CWiaDataCallback class instance";
                }

                /* Release the IWiaDataTransfer */
                pWiaDataTransfer->Release();
                pWiaDataTransfer = nullptr;
            }
            else {
                LOG_SAVE << "pWiaItem->QueryInterface failed on IID_IWiaDataTransfer";
            }
        }

        /* Release the IWiaPropertyStorage */
        pWiaPropertyStorage->Release();
        pWiaPropertyStorage = nullptr;
    }
    else {
        LOG_SAVE << "pWiaItem->QueryInterface failed on IID_IWiaPropertyStorage";
    }

    return hr;
}

HRESULT Crus1scan::wia_get_events(IWiaItem* pWiaItem)
{
	LOG_SAVE << "WiaGetEvents";

    if (nullptr == pWiaItem)
        return E_INVALIDARG;

    IWiaPropertyStorage* pWiaPropertyStorage = nullptr;
    HRESULT hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, reinterpret_cast<void**>(&pWiaPropertyStorage));
    if (SUCCEEDED(hr)) {
	    act_event cWIA_Device;
	    /* Declare PROPSPECs and PROPVARIANTs, and initialize them to zero. */
        PROPSPEC prop_spec[4] = {};
        PROPVARIANT prop_var[4] = {};

        // How many properties are we querying for?
	    constexpr ULONG property_count = std::size(prop_spec);

        // Define which properties we want to read: Device ID.
        // This is what we'd use to create the device.
        prop_spec[0].ulKind = PRSPEC_PROPID;
        prop_spec[0].propid = WIA_IPA_FULL_ITEM_NAME;

        prop_spec[1].ulKind = PRSPEC_PROPID;
        prop_spec[1].propid = WIA_DIP_DEV_ID;

        /* Device name */
        prop_spec[2].ulKind = PRSPEC_PROPID;
        prop_spec[2].propid = WIA_DIP_DEV_NAME;

        /* Device description */
        prop_spec[3].ulKind = PRSPEC_PROPID;
        prop_spec[3].propid = WIA_DIP_DEV_DESC;

        /* property values */
        hr = pWiaPropertyStorage->ReadMultiple(property_count, prop_spec, prop_var);
        if (SUCCEEDED(hr)) {
            // IWiaPropertyStorage::ReadMultiple will return S_FALSE if some
            // properties could not be read, so we have to check the return
            // types for each requested item.

            // Check the return type for the device ID
            if (VT_BSTR == prop_var[1].vt) {
                LOG_SAVE << "Item Name: " << prop_var[1].bstrVal;
                cWIA_Device.device_guid = bstr_to_std_string(prop_var[1].bstrVal);
                cWIA_Device.index_of_device = dev_index_;
                ++dev_index_;
            }
            if (VT_BSTR == prop_var[2].vt) {
                LOG_SAVE << "Item Name: " << prop_var[2].bstrVal;
                cWIA_Device.device_name = bstr_to_std_string(prop_var[2].bstrVal);
            }
            if (VT_BSTR == prop_var[3].vt) {
                LOG_SAVE << "Item Name: " << prop_var[3].bstrVal;
            }

            /* Free the returned PROPVARIANTs */
            FreePropVariantArray(property_count, prop_var);
        }
        else {
            LOG_SAVE << "Error calling IWiaPropertyStorage::ReadMultiple";
        }

        IEnumWIA_DEV_CAPS* p_enum_dev_caps;
	    ULONG l_count;
        ULONG n_fetched;

	    hr = pWiaItem->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS, &p_enum_dev_caps);
        if (hr == S_OK) {
            LOG_SAVE << "EnumDeviceCapabilities - S_OK";
            hr = p_enum_dev_caps->GetCount(&l_count);
            if (l_count != 0) {
	            CString event_str;
	            LOG_SAVE << "Count != 0";
                WIA_DEV_CAP* p_dev_caps = new WIA_DEV_CAP[l_count];

                p_enum_dev_caps->Next(l_count, p_dev_caps, &n_fetched);                
                for (int i = 0; i < n_fetched; i++)
                {                    
                    if (1 == p_dev_caps[i].ulFlags || 3 == p_dev_caps[i].ulFlags) /* "1" - connected/disconnected, "3" means Button
                         * pDevCaps[i].ulFlags |= WIA_DEVICE_PROPERTY_BROADCAST_CONNECT, WIA_DEVICE_FLAGS_NO_BROADCAST_CONNECT;
                         * Приложение может определить, является ли событие типом действия или типом уведомления - значение ulFlags структуры WIA_DEV_CAP, возвращаемое в перечислении событий.
                         * WIA_IS_DEFAULT_HANDLER	The currently registered handler should be used.This is the only valid value when enumerating event handlers.It is not a valid value when enumerating event capabilities of a device.
                         * WIA_ACTION_EVENT	The event is of the action type, so programs that use persistent registration APIs, IWiaDevMgr::RegisterEventCallbackProgram and IWiaDevMgr::RegisterEventCallbackCLSID, can receive it.
                         * WIA_NOTIFICATION_EVENT	The event is of the notification type, so programs that use the runtime registration function, IWiaDevMgr::RegisterEventCallbackInterface, can receive it.
                         */
                    {
                        /* eventStr = pDevCaps[i].bstrName;
                         * rasa
                         * if (mfcElements.comboBoxEvents) mfcElements.comboBoxEvents->AddString(eventStr);
                         */
                        cWIA_Device.btn_event_name = bstr_to_u32(p_dev_caps[i].bstrName);

                        LOG_SAVE << "bstrName" << event_str;
                        // eventStr = pDevCaps[i].bstrCommandline;
                        // eventStr = pDevCaps[i].bstrDescription;
                        // eventStr = pDevCaps[i].bstrIcon;

                        OLECHAR* guid_string;
                        StringFromCLSID(p_dev_caps[i].guid, &guid_string);
                        event_str = guid_string;
                        LOG_SAVE << "guidString" << event_str;
                        cWIA_Device.event_guid = event_str;

                        /* ensure memory is freed */
                        ::CoTaskMemFree(guid_string);

                        if(!cWIA_Device.device_guid.empty())
                            act_events.push_back(cWIA_Device);

                        cWIA_Device.btn_event_name.clear();
                        cWIA_Device.event_guid.clear();
                    }
                }
            }
        }

        pWiaPropertyStorage->Release();
        pWiaPropertyStorage = nullptr;
    }

    return hr;
}

HRESULT Crus1scan::get_from_device(IWiaDevMgr* pWiaDevMgr, const BSTR bstr_device_id, IWiaItem** ppWiaDevice) {

    if (nullptr == pWiaDevMgr || nullptr == bstr_device_id || nullptr == ppWiaDevice)
        return E_INVALIDARG;

    /* Initialize out variables */
    *ppWiaDevice = nullptr;

    /* Create the WIA Device */
    const HRESULT hr = pWiaDevMgr->CreateDevice(bstr_device_id, ppWiaDevice);

    LOG_SAVE << pWiaDevMgr << "GetFromDevice";
    LOG_SAVE << bstr_device_id;
    LOG_SAVE << ppWiaDevice;

    if (FAILED(hr))
        LOG_SAVE << "Error calling IWiaDevMgr::CreateDevice";

    return hr;
}

HRESULT Crus1scan::create_wia_device_manager(IWiaDevMgr** ppWiaDevMgr) {

    if (nullptr == ppWiaDevMgr)
        return E_INVALIDARG;

    /* Initialize out variables */
    *ppWiaDevMgr = nullptr;

    /* Create an instance of the device manager */
    const HRESULT hr = CoCreateInstance(CLSID_WiaDevMgr, nullptr, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, reinterpret_cast<void**>(ppWiaDevMgr));
    if (FAILED(hr))
        LOG_SAVE << "CoCreateInstance failed on CLSID_WiaDevMgr";

    return hr;
}

HRESULT Crus1scan::get_wia_prop_dev_id(IWiaPropertyStorage* pWiaPropertyStorage) {

    if (nullptr == pWiaPropertyStorage)
        return E_INVALIDARG;

    /* Declare PROPSPECs and PROPVARIANTs, and initialize them to zero */
    PROPSPEC prop_spec[3] = {};
    PROPVARIANT prop_var[3] = {};

    /* How many properties are we querying for? */
    constexpr ULONG property_count = std::size(prop_spec);

    // Define which properties we want to read: Device ID.
    // This is what we'd use to create the device.
    prop_spec[0].ulKind = PRSPEC_PROPID;
    prop_spec[0].propid = WIA_DIP_DEV_ID;

    /* Device name */
    prop_spec[1].ulKind = PRSPEC_PROPID;
    prop_spec[1].propid = WIA_DIP_DEV_NAME;

    /* Device description */
    prop_spec[2].ulKind = PRSPEC_PROPID;
    prop_spec[2].propid = WIA_DIP_DEV_DESC;

    /* property values */
    const HRESULT hr = pWiaPropertyStorage->ReadMultiple(property_count, prop_spec, prop_var);
    if (SUCCEEDED(hr)) {
        /* IWiaPropertyStorage::ReadMultiple will return S_FALSE if some
         * properties could not be read, so we have to check the return
         * types for each requested item.
		 */

        /* return type for the device ID */
        if (VT_BSTR == prop_var[0].vt) {
            LOG_SAVE << "WIA_DIP_DEV_ID: " << prop_var[0].bstrVal;
        }

        /* return type for the device name */
        if (VT_BSTR == prop_var[1].vt) {
            LOG_SAVE << "WIA_DIP_DEV_NAME: " << prop_var[1].bstrVal;
        }

        /* return type for the device description */
        if (VT_BSTR == prop_var[2].vt) {
            LOG_SAVE << "WIA_DIP_DEV_DESC: " << prop_var[2].bstrVal;
        }

        /* Free the returned PROPVARIANTs */
        FreePropVariantArray(property_count, prop_var);
    }
    else {
        LOG_SAVE << "Error calling IWiaPropertyStorage::ReadMultiple";
    }

    return hr;
}

HRESULT Crus1scan::enumerate_items(IWiaItem* pWiaItem) {
    LOG_SAVE << "Enumerating WIA Items: " << pWiaItem;
    
    if (nullptr == pWiaItem)
        return E_INVALIDARG;

    /* Get the item type for this item */
    LONG l_item_type = 0;
    HRESULT hr = pWiaItem->GetItemType(&l_item_type);

    LOG_SAVE << "Item Type is:" << l_item_type;

    /* If this is an image - transfer it */
    if (l_item_type & WiaItemTypeImage) {
        LOG_SAVE << "Item Type is Image:" << l_item_type;
////////////////////////////////////////////////////////////////////////////////////////////////
//TODO: hr = TransferWiaItem(pWiaItem);
        hr = S_OK;
////////////////////////////////////////////////////////////////////////////////////////////////    
    }
    // if ( lItemType & WiaItemTypeFolder || lItemType & WiaItemTypeHasAttachments)
    // If it is a folder and has no attachments (e.g. photos in a smartphone), enumerate its children 
    if (!(l_item_type & WiaItemTypeHasAttachments)) {
        LOG_SAVE << "Item Type is Folder. Enumerating children of " << l_item_type;

        /* Get the child item enumerator for this item */
        IEnumWiaItem* pEnumWiaItem = nullptr;
        hr = pWiaItem->EnumChildItems(&pEnumWiaItem);
        if (SUCCEEDED(hr)) {
            // We will loop until we get an error or pEnumWiaItem->Next returns
            // S_FALSE to signal the end of the list.
            while (S_OK == hr) {
                // Get the next child item
                IWiaItem* pChildWiaItem = nullptr;
                hr = pEnumWiaItem->Next(1, &pChildWiaItem, nullptr);

                // pEnumWiaItem->Next will return S_FALSE when the list is
                // exhausted, so check for S_OK before using the returned value.
                if (S_OK == hr) {
                    wia_get_events(pWiaItem);

                	/* recurse into this item */
                    hr = enumerate_items(pChildWiaItem);

                    /* Release this item */
                    pChildWiaItem->Release();
                    pChildWiaItem = nullptr;
                }
                else if (FAILED(hr))
                    LOG_SAVE << "Error calling pEnumWiaItem->Next";
            }

            /* If the result of the enumeration is S_FALSE, since this is normal, we will change it to S_OK */
            if (S_FALSE == hr)
                hr = S_OK;

            /* Release the enumerator */
            pEnumWiaItem->Release();
            pEnumWiaItem = nullptr;
        }
    }
    return  hr;
}

HRESULT Crus1scan::transfer_image(IWiaItem* pWiaItem) {
    LOG_SAVE << pWiaItem;
    
    if (nullptr == pWiaItem)
        return E_INVALIDARG;

    /* Get the item type for this item */
    LONG lItemType = 0;
    HRESULT hr = pWiaItem->GetItemType(&lItemType);

    /* If this is an image, transfer it */
    if (lItemType & WiaItemTypeImage) {
        LOG_SAVE << lItemType;
        // TODO: //////////////////////////////////////////////////////////////////////////////////////////////
        hr = transfer_wia_item(pWiaItem);
        hr = S_OK;
        ////////////////////////////////////////////////////////////////////////////////////////////////    
    }
    return  hr;
}

HRESULT Crus1scan::create_wia_dev(IWiaDevMgr* pWiaDevMgr, IWiaPropertyStorage* pWiaPropertyStorage) {

    if (nullptr == pWiaPropertyStorage)
        return E_INVALIDARG;

    /* Declare PROPSPECs and PROPVARIANTs, and initialize them to zero */
    PROPSPEC prop_spec[1] = {};
    PROPVARIANT prop_var[1] = {};

    /*/ How many properties are we querying for? */
    constexpr ULONG property_count = std::size(prop_spec);

    // Define which properties we want to read.  We want the
    // device ID, so we can create the device.
    prop_spec[0].ulKind = PRSPEC_PROPID;
    prop_spec[0].propid = WIA_DIP_DEV_ID;

    /* the property values */
    HRESULT hr = pWiaPropertyStorage->ReadMultiple(property_count, prop_spec, prop_var);
    if (SUCCEEDED(hr)) { 
        /* Check the return type for the device ID to make sure this property was actually read */
//      if (VT_BSTR == PropVar[0].vt)
        { 
            // Create the WIA device, which results in obtaining the root IWiaItem
            IWiaItem* pWiaRootItem = nullptr;
            hr = get_from_device(pWiaDevMgr, prop_var[0].bstrVal, &pWiaRootItem);
            if (SUCCEEDED(hr)) {
                LOG_SAVE << "Reading buttons WiaGetEvents(pWiaRootItem)";

//                WiaGetEvents(pWiaRootItem);
                // Recursively enumerate the items
                // (This function will also do some stuff with each item)
 //               LOG_SAVE << "EnumerateItems(pWiaRootItem)";
 // TODO: ////////////////////////////////////////////////////////////////////////////////////////////// 
                  hr = enumerate_items(pWiaRootItem);                                    // 09.10.2023 TRANSFER IS rasa
//                hr = TransferImage(pWiaRootItem);                                      // 31.07.2024 TRANSFER IS rasa
//                WiaWrap::CComPtrArray<IStream> ppStream;
//                hr = WiaWrap::WiaGetEvents(mfcElements, StiDeviceTypeDefault, 0, WIA_INTENT_NONE, pWiaDevMgr, pWiaRootItem, nullptr,
//                                           nullptr, nullptr, &ppStream.Count(), &ppStream);
                // Free the root item
                pWiaRootItem->Release();
                pWiaRootItem = nullptr;
            }
            else {
                LOG_SAVE << "Error calling CreateWiaDevice";
            }
      }

        /* Free the returned PROPVARIANTs */
       FreePropVariantArray(property_count, prop_var);
    }
    else {
        LOG_SAVE << "Error calling IWiaPropertyStorage::ReadMultiple";
    }

    /* Return the result of all of this */
    return hr;
}

HRESULT Crus1scan::enumerate_wia_devices(IWiaDevMgr* pWiaDevMgr) {
    if (nullptr == pWiaDevMgr)
        return E_INVALIDARG;

    IEnumWIA_DEV_INFO* pWiaEnumDevInfo = nullptr;
    HRESULT hr = pWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &pWiaEnumDevInfo);
    
    if (SUCCEEDED(hr)) {
        /* Reset the device enumerator to the beginning of the list */
        hr = pWiaEnumDevInfo->Reset();
        if (SUCCEEDED(hr)) {
            // We will loop until we get an error or pWiaEnumDevInfo->Next returns
            // S_FALSE to signal the end of the list.
            while (S_OK == hr) {
                /* Get the next device's property storage interface pointer */
                IWiaPropertyStorage* pWiaPropertyStorage = nullptr;
                hr = pWiaEnumDevInfo->Next(1, &pWiaPropertyStorage, nullptr);

                // pWiaEnumDevInfo->Next will return S_FALSE when the list is
                // exhausted, so check for S_OK before using the returned value.
                if (hr == S_OK) {
                    // Read the device's IWiaPropertyStorage*
                    LOG_SAVE << "Getting WIA devices names to create them:";
                    get_wia_prop_dev_id(pWiaPropertyStorage);
                    LOG_SAVE << "ReadWiaProperties(pWiaPropertyStorage);";

                    // Call a helper function to create the device and get buttons
                    create_wia_dev(pWiaDevMgr, pWiaPropertyStorage);

                    // Release the device's IWiaPropertyStorage*
                    pWiaPropertyStorage->Release();
                    pWiaPropertyStorage = nullptr;
                }
                else if (FAILED(hr)) {
                    LOG_SAVE << "Error calling pWiaEnumDevInfo->Next";
                }
            }

            // If the result of the enumeration is S_FALSE, since this
            // is normal, we will change it to S_OK.
            if (S_FALSE == hr) {
                hr = S_OK;
            }
        }
        else {
            LOG_SAVE << "Error calling IEnumWIA_DEV_INFO::Reset()";
        }

        // Release the enumerator
        pWiaEnumDevInfo->Release();
        pWiaEnumDevInfo = nullptr;
    }
    else {
        LOG_SAVE << "Error calling IWiaDevMgr::EnumDeviceInfo";
    }

    /* Return the result of the enumeration */
    return hr;
}

Crus1scan::~Crus1scan(void) { }