
// FTech_StCam.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CFTech_StCamApp:
// See FTech_StCam.cpp for the implementation of this class
//

class CFTech_StCamApp : public CWinApp
{
public:
	CFTech_StCamApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CFTech_StCamApp theApp;