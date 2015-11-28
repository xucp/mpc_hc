/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "PPageBase.h"
#include "PlayerListCtrl.h"
#include "StaticLink.h"
#include "WinHotkeyCtrl.h"
#include "vkCodes.h"

// CPPageAccelTbl dialog

class CPPageAccelTbl : public CPPageBase
{
private:
    enum {
        COL_CMD,
        COL_KEY,
        COL_ID,
        COL_MOUSE,
        COL_MOUSE_FS,
        COL_APPCMD,
        COL_RMCMD,
        COL_RMREPCNT
    };

    enum { APPCOMMAND_LAST = APPCOMMAND_DWM_FLIP3D };

    CList<wmcmd> m_wmcmds;
    int m_counter;

    CPlayerListCtrl m_list;
    BOOL m_fWinLirc;
    CString m_WinLircAddr;
    CEdit m_WinLircEdit;
    CStaticLink m_WinLircLink;
    BOOL m_fUIce;
    CString m_UIceAddr;
    CEdit m_UIceEdit;
    CStaticLink m_UIceLink;
    UINT_PTR m_nStatusTimerID;
    BOOL m_fGlobalMedia;

    static CString MakeAccelModLabel(BYTE fVirt);
    static CString MakeAccelShortcutLabel(const ACCEL& a);
    static CString MakeMouseButtonLabel(UINT mouse);
    static CString MakeAppCommandLabel(UINT id);

    void SetupList();

public:
    DECLARE_DYNAMIC(CPPageAccelTbl)

    CPPageAccelTbl();
    virtual ~CPPageAccelTbl();

    // Dialog Data
    enum { IDD = IDD_PPAGEACCELTBL };

    static CString MakeAccelShortcutLabel(UINT id);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnBeginListLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoListLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEndListLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedSelectAll();
    afx_msg void OnBnClickedReset();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    virtual void OnCancel();
};
