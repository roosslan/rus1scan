#pragma once
#include "StdAfx.h"

CWiaDataCallback::~CWiaDataCallback() {};

// IUnknown functions
HRESULT CALLBACK CWiaDataCallback::QueryInterface(REFIID riid, void** ppvObject)
{
	// Validate arguments
	if (NULL == ppvObject)
	{
		return E_INVALIDARG;
	}

	// Return the appropropriate interface
	if (IsEqualIID(riid, IID_IUnknown))
	{
		*ppvObject = static_cast<CWiaDataCallback*>(this);
	}
	else if (IsEqualIID(riid, IID_IWiaDataCallback))
	{
		*ppvObject = static_cast<CWiaDataCallback*>(this);
	}
	else
	{
		*ppvObject = NULL;
		return (E_NOINTERFACE);
	}

	// Increment the reference count before we return the interface
	reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
	return S_OK;
}
ULONG CALLBACK CWiaDataCallback::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}
ULONG CALLBACK CWiaDataCallback::Release()
{
	LONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
	{
		delete this;
	}
	return cRef;
}

// IWiaDataCallback functions
HRESULT CALLBACK CWiaDataCallback::BandedDataCallback(
	LONG lMessage,
	LONG lStatus,
	LONG lPercentComplete,
	LONG lOffset,
	LONG lLength,
	LONG lReserved,
	LONG lResLength,
	BYTE* pbBuffer)
{
	UNREFERENCED_PARAMETER(lMessage);
	UNREFERENCED_PARAMETER(lStatus);
	UNREFERENCED_PARAMETER(lPercentComplete);
	UNREFERENCED_PARAMETER(lOffset);
	UNREFERENCED_PARAMETER(lLength);
	UNREFERENCED_PARAMETER(lReserved);
	UNREFERENCED_PARAMETER(lResLength);
	UNREFERENCED_PARAMETER(pbBuffer);
	switch (lMessage)
	{
	case IT_MSG_STATUS:
		LOG_SAVE << "lPercentComplete: " << lPercentComplete;
		break;
	}

	return S_OK;
};

