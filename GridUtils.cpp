// ======================================================================
// WinGridStrument - a Windows touchscreen musical instrument
// Copyright(C) 2020 Roger Allen
// 
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// ======================================================================
#include "GridUtils.h"

void AlertExit(HWND hWnd, LPCTSTR text) {
    std::wcout << "ERROR: " << text << "\nUnable to recover. Program will close." << std::endl;
    std::wostringstream text1;
    text1 << "ERROR: " << text << "\nUnable to recover. Program will close.";
    MessageBox(hWnd, text1.str().c_str(), NULL, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    exit(99);
}