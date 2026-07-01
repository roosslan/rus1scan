#ifndef RUS1SCAN_H
#define RUS1SCAN_H

#include "stdafx.h"

#include "helper_funcs.h"

class Crus1scan {
	/* Serves as index of WIA-devices to bind Events */
	int dev_index_ = 0;
public:
	std::vector<act_event> act_events;
	Crus1scan(IWiaDevMgr** device_manager);
	~Crus1scan(void);

	HRESULT create_wia_device_manager(IWiaDevMgr** ppWiaDevMgr);
	HRESULT enumerate_wia_devices(IWiaDevMgr* pWiaDevMgr);
	HRESULT get_wia_prop_dev_id(IWiaPropertyStorage* pWiaPropertyStorage);
	HRESULT wia_get_events(IWiaItem* pWiaItem);
	HRESULT get_from_device(IWiaDevMgr* pWiaDevMgr, BSTR bstr_device_id, IWiaItem** ppWiaDevice);
	HRESULT create_wia_dev(IWiaDevMgr* pWiaDevMgr, IWiaPropertyStorage* pWiaPropertyStorage);
	HRESULT enumerate_items(IWiaItem* pWiaItem);
	HRESULT transfer_image(IWiaItem* pWiaItem);
	HRESULT transfer_wia_item(IWiaItem* pIWiaItem);
};

#endif