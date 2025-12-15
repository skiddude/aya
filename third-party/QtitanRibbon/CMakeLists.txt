set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set_source_files_properties(src/ribbondsgn/QtnRibbonStyleDsgnPlugin.cpp PROPERTIES SKIP_AUTOMOC TRUE)

add_library(QtitanRibbon STATIC
  src/include/QtitanDef.h
  src/include/QtitanRibbon.h
  src/include/QtnCommonStyle.h
  src/include/QtnOfficePopupColorButton.h
  src/include/QtnOfficePopupMenu.h
  src/include/QtnOfficePopupWindow.h
  src/include/QtnOfficeStyle.h
  src/include/QtnRibbonBackstageView.h
  src/include/QtnRibbonBar.h
  src/include/QtnRibbonButtonControls.h
  src/include/QtnRibbonCustomizeDialog.h
  src/include/QtnRibbonCustomizeManager.h
  src/include/QtnRibbonCustomizePage.h
  src/include/QtnRibbonDef.h
  src/include/QtnRibbonGallery.h
  src/include/QtnRibbonGalleryControls.h
  src/include/QtnRibbonGroup.h
  src/include/QtnRibbonInputControls.h
  src/include/QtnRibbonMainWindow.h
  src/include/QtnRibbonPage.h
  src/include/QtnRibbonQuickAccessBar.h
  src/include/QtnRibbonSliderPane.h
  src/include/QtnRibbonStatusBar.h
  src/include/QtnRibbonStyle.h
  src/include/QtnRibbonSystemPopupBar.h
  src/include/QtnRibbonToolTip.h
  src/include/QtnRibbonWorkspace.h
  src/include/QtnStyleHelpers.h

  src/base/QtitanDef.h
  src/base/QtitanMSVSDebug.h

  # Ribbon
  src/ribbon/QtnOfficeFrameHelper_win.cpp
  src/ribbon/QtnOfficeFrameHelper_win.h
  src/ribbon/QtnOfficeFrameHelper.h
  src/ribbon/QtnOfficePopupColorButton.cpp
  src/ribbon/QtnOfficePopupColorButton.h
  src/ribbon/QtnOfficePopupMenu.cpp
  src/ribbon/QtnOfficePopupMenu.h
  src/ribbon/QtnOfficePopupWindow.cpp
  src/ribbon/QtnOfficePopupWindow.h
  src/ribbon/QtnRibbonBackstageView.cpp
  src/ribbon/QtnRibbonBackstageView.h
  src/ribbon/QtnRibbonBackstageViewPrivate.h
  src/ribbon/QtnRibbonBar.cpp
  src/ribbon/QtnRibbonBar.h
  src/ribbon/QtnRibbonBarCustomizePage.ui
  src/ribbon/QtnRibbonBarPrivate.h
  src/ribbon/QtnRibbonButtonControls.cpp
  src/ribbon/QtnRibbonButtonControls.h
  src/ribbon/QtnRibbonButtonPrivate.cpp
  src/ribbon/QtnRibbonButtonPrivate.h
  src/ribbon/QtnRibbonControls.cpp
  src/ribbon/QtnRibbonControls.h
  src/ribbon/QtnRibbonControlsPrivate.h
  src/ribbon/QtnRibbonCustomizeDialog.cpp
  src/ribbon/QtnRibbonCustomizeDialog.h
  src/ribbon/QtnRibbonCustomizeDialogPrivate.h
  src/ribbon/QtnRibbonCustomizeManager.cpp
  src/ribbon/QtnRibbonCustomizeManager.h
  src/ribbon/QtnRibbonCustomizeManagerPrivate.h
  src/ribbon/QtnRibbonCustomizePage.cpp
  src/ribbon/QtnRibbonCustomizePage.h
  src/ribbon/QtnRibbonDef.cpp
  src/ribbon/QtnRibbonDef.h
  src/ribbon/QtnRibbonGallery.cpp
  src/ribbon/QtnRibbonGallery.h
  src/ribbon/QtnRibbonGalleryControls.cpp
  src/ribbon/QtnRibbonGalleryControls.h
  src/ribbon/QtnRibbonGalleryPrivate.h
  src/ribbon/QtnRibbonGroup.cpp
  src/ribbon/QtnRibbonGroup.h
  src/ribbon/QtnRibbonGroupPrivate.h
  src/ribbon/QtnRibbonInputControls.cpp
  src/ribbon/QtnRibbonInputControls.h
  src/ribbon/QtnRibbonMainWindow.cpp
  src/ribbon/QtnRibbonMainWindow.h
  src/ribbon/QtnRibbonPage.cpp
  src/ribbon/QtnRibbonPage.h
  src/ribbon/QtnRibbonPagePrivate.h
  src/ribbon/QtnRibbonPrivate.cpp
  src/ribbon/QtnRibbonPrivate.h
  src/ribbon/QtnRibbonQuickAccessBar.cpp
  src/ribbon/QtnRibbonQuickAccessBar.h
  src/ribbon/QtnRibbonQuickAccessBarCustomizePage.ui
  src/ribbon/QtnRibbonRenameDialog.ui
  src/ribbon/QtnRibbonSliderPane.cpp
  src/ribbon/QtnRibbonSliderPane.h
  src/ribbon/QtnRibbonStatusBar.cpp
  src/ribbon/QtnRibbonStatusBar.h
  src/ribbon/QtnRibbonSystemPopupBar.cpp
  src/ribbon/QtnRibbonSystemPopupBar.h
  src/ribbon/QtnRibbonTabBar.cpp
  src/ribbon/QtnRibbonTabBar.h
  src/ribbon/QtnRibbonToolTip.cpp
  src/ribbon/QtnRibbonToolTip.h
  src/ribbon/QtnRibbonWorkspace.cpp
  src/ribbon/QtnRibbonWorkspace.h

  # Ribbon Design
  src/ribbondsgn/QtitanRibbon_Backstage_Page.ui
  src/ribbondsgn/QtitanRibbon_Main_Window.ui
  src/ribbondsgn/QtnCommandDsgn.cpp
  src/ribbondsgn/QtnCommandDsgn.h
  src/ribbondsgn/QtnMainWindowDsgnContainer.cpp
  src/ribbondsgn/QtnMainWindowDsgnContainer.h
  src/ribbondsgn/QtnMainWindowDsgnTaskMenu.cpp
  src/ribbondsgn/QtnMainWindowDsgnTaskMenu.h
  src/ribbondsgn/QtnRibbonBackstageViewDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonBackstageViewDsgnPlugin.h
  src/ribbondsgn/QtnRibbonBarDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonBarDsgnPlugin.h
  src/ribbondsgn/QtnRibbonDsgnContainer.cpp
  src/ribbondsgn/QtnRibbonDsgnContainer.h
  src/ribbondsgn/QtnRibbonDsgnPlugins.cpp
  src/ribbondsgn/QtnRibbonDsgnPlugins.h
  src/ribbondsgn/QtnRibbonDsgnTaskMenu.cpp
  src/ribbondsgn/QtnRibbonDsgnTaskMenu.h
  src/ribbondsgn/QtnRibbonGroupDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonGroupDsgnPlugin.h
  src/ribbondsgn/QtnRibbonMainWindowDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonMainWindowDsgnPlugin.h
  src/ribbondsgn/QtnRibbonPageDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonPageDsgnPlugin.h
  src/ribbondsgn/QtnRibbonStyleDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonStyleDsgnPlugin.h
  src/ribbondsgn/QtnRibbonWorkspaceDsgnPlugin.cpp
  src/ribbondsgn/QtnRibbonWorkspaceDsgnPlugin.h
  src/ribbondsgn/QtnStatusBarDsgnPlugin.cpp
  src/ribbondsgn/QtnStatusBarDsgnPlugin.h
  src/ribbondsgn/ribbondsgn.pro

  # Ribbon Styles
  src/styles/QtnCommonPaintManager.cpp
  src/styles/QtnCommonStyle_linux.cpp
  src/styles/QtnCommonStyle_win.cpp
  src/styles/QtnCommonStyle.cpp
  src/styles/QtnCommonStyle.h
  src/styles/QtnCommonStylePrivate.h
  src/styles/QtnOfficePaintManager.cpp
  src/styles/QtnOfficeStyle.cpp
  src/styles/QtnOfficeStyle.h
  src/styles/QtnOfficeStylePrivate.h
  src/styles/QtnPopupHelpers.cpp
  src/styles/QtnPopupHelpers.h
  src/styles/QtnRibbonPaintManager.cpp
  src/styles/QtnRibbonStyle.cpp
  src/styles/QtnRibbonStyle.h
  src/styles/QtnRibbonStylePrivate.h
  src/styles/QtnStyleHelpers.cpp
  src/styles/QtnStyleHelpers.h
  src/styles/styles.pri
)

# hack
if(AYA_OS_LINUX)
  target_include_directories(QtitanRibbon PUBLIC "/usr/include/qt/QtDesigner")
endif()

target_compile_definitions(QtitanRibbon PRIVATE QTITAN_STATIC QTITAN_LIBRARY QTITAN_LIBRARY_STATIC)
target_include_directories(QtitanRibbon
  PRIVATE
    # TODO: do we still need these two?
    ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
    ${QtWidgets_PRIVATE_INCLUDE_DIRS}

    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ribbon
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ribbondsgn
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(QtitanRibbon PRIVATE
  Qt6::Widgets
  Qt6::Core
  Qt6::Concurrent
  Qt6::Gui
  Qt6::Designer
)

if(AYA_OS_WINDOWS)
  target_link_libraries(QtitanRibbon PRIVATE Comctl32.lib)
endif()