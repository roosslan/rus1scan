#pragma once

#ifndef WIADATACALLBACK_H
#define WIADATACALLBACK_H

class CWiaDataCallback : public IWiaDataCallback
{
private:
    // Reference count
    LONG m_cRef;

private:
    // No implementation
    CWiaDataCallback(const CWiaDataCallback&);
    CWiaDataCallback& operator=(const CWiaDataCallback&);

public:
    //
    // Constructor and destructor
    CWiaDataCallback() : m_cRef(1) {}
    ~CWiaDataCallback();
    // IUnknown functions
    HRESULT CALLBACK QueryInterface(REFIID riid, void** ppvObject);
    ULONG CALLBACK AddRef();
    ULONG CALLBACK Release();

    // IWiaDataCallback functions
    HRESULT CALLBACK BandedDataCallback(
        LONG lMessage,
        LONG lStatus,
        LONG lPercentComplete,
        LONG lOffset,
        LONG lLength,
        LONG lReserved,
        LONG lResLength,
        BYTE* pbBuffer);
};

#endif