// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <QMessageBox>
#include <QProgressDialog>
#include "citra_qt/configuration/configure_system.h"
#include "citra_qt/uisettings.h"
#include "core/core.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/cfg/cfg.h"
#include "core/hle/service/ptm/ptm.h"
#include "core/hw/aes/key.h"
#include "core/settings.h"
#include "ui_configure_system.h"

static const std::array<int, 12> days_in_month = {{
    31,
    29,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31,
}};

static const std::array<const char*, 187> country_names = {
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "Japan"),
    "",
    "",
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "Anguilla"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Antigua and Barbuda"), // 0-9
    QT_TRANSLATE_NOOP("ConfigureSystem", "Argentina"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Aruba"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bahamas"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Barbados"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Belize"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bolivia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Brazil"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "British Virgin Islands"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Canada"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Cayman Islands"), // 10-19
    QT_TRANSLATE_NOOP("ConfigureSystem", "Chile"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Colombia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Costa Rica"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Dominica"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Dominican Republic"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Ecuador"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "El Salvador"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "French Guiana"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Grenada"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Guadeloupe"), // 20-29
    QT_TRANSLATE_NOOP("ConfigureSystem", "Guatemala"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Guyana"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Haiti"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Honduras"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Jamaica"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Martinique"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Mexico"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Montserrat"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Netherlands Antilles"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Nicaragua"), // 30-39
    QT_TRANSLATE_NOOP("ConfigureSystem", "Panama"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Paraguay"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Peru"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Saint Kitts and Nevis"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Saint Lucia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Saint Vincent and the Grenadines"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Suriname"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Trinidad and Tobago"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Turks and Caicos Islands"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "United States"), // 40-49
    QT_TRANSLATE_NOOP("ConfigureSystem", "Uruguay"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "US Virgin Islands"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Venezuela"),
    "",
    "",
    "",
    "",
    "",
    "",
    "", // 50-59
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "Albania"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Australia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Austria"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Belgium"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bosnia and Herzegovina"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Botswana"), // 60-69
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bulgaria"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Croatia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Cyprus"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Czech Republic"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Denmark"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Estonia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Finland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "France"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Germany"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Greece"), // 70-79
    QT_TRANSLATE_NOOP("ConfigureSystem", "Hungary"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Iceland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Ireland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Italy"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Latvia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Lesotho"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Liechtenstein"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Lithuania"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Luxembourg"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Macedonia"), // 80-89
    QT_TRANSLATE_NOOP("ConfigureSystem", "Malta"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Montenegro"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Mozambique"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Namibia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Netherlands"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "New Zealand"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Norway"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Poland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Portugal"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Romania"), // 90-99
    QT_TRANSLATE_NOOP("ConfigureSystem", "Russia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Serbia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Slovakia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Slovenia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "South Africa"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Spain"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Swaziland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Sweden"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Switzerland"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Turkey"), // 100-109
    QT_TRANSLATE_NOOP("ConfigureSystem", "United Kingdom"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Zambia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Zimbabwe"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Azerbaijan"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Mauritania"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Mali"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Niger"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Chad"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Sudan"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Eritrea"), // 110-119
    QT_TRANSLATE_NOOP("ConfigureSystem", "Djibouti"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Somalia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Andorra"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Gibraltar"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Guernsey"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Isle of Man"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Jersey"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Monaco"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Taiwan"),
    "", // 120-129
    "",
    "",
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "South Korea"),
    "",
    "",
    "", // 130-139
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "Hong Kong"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Macau"),
    "",
    "",
    "",
    "", // 140-149
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "Indonesia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Singapore"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Thailand"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Philippines"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Malaysia"),
    "",
    "",
    "", // 150-159
    QT_TRANSLATE_NOOP("ConfigureSystem", "China"),
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "United Arab Emirates"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "India"), // 160-169
    QT_TRANSLATE_NOOP("ConfigureSystem", "Egypt"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Oman"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Qatar"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Kuwait"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Saudi Arabia"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Syria"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bahrain"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Jordan"),
    "",
    "", // 170-179
    "",
    "",
    "",
    "",
    QT_TRANSLATE_NOOP("ConfigureSystem", "San Marino"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Vatican City"),
    QT_TRANSLATE_NOOP("ConfigureSystem", "Bermuda"), // 180-186
};

struct Title {
    enum Mode { All, Recommended, Minimal };
    std::string name;
    std::array<u32, 6> lower_title_id;
    Mode mode = Mode::All;
};

static const std::array<Title, 9> systemFirmware = {
    {{"Safe Mode Native Firmware",
      {{0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x00000003}},
      Title::Mode::Minimal},
     {"New_3DS Safe Mode Native Firmware",
      {{0x20000003, 0x20000003, 0x20000003, 0x20000003, 0x20000003, 0x20000003}},
      Title::Mode::Minimal},
     {"Native Firmware",
      {{0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002}},
      Title::Mode::Minimal},
     {"New_3DS Native Firmware",
      {{0x20000002, 0x20000002, 0x20000002, 0x20000002, 0x20000002, 0x20000002}},
      Title::Mode::Minimal}}};
static const std::array<Title, 17> systemApplications = {
    {{"System Settings",
      {{0x00020000, 0x00021000, 0x00022000, 0x00026000, 0x00027000, 0x00028000}},
      Title::Mode::All},
     {"Download Play",
      {{0x00020100, 0x00021100, 0x00022100, 0x00026100, 0x00027100, 0x00028100}},
      Title::Mode::Recommended},
     {"Activity Log",
      {{0x00020200, 0x00021200, 0x00022200, 0x00026200, 0x00027200, 0x00028200}},
      Title::Mode::All},
     {"Health and Safety Information",
      {{0x00020300, 0x00021300, 0x00022300, 0x00026300, 0x00027300, 0x00028300}},
      Title::Mode::All},
     {"New_3DS Health and Safety Information",
      {{0x20020300, 0x20021300, 0x20022300, 0x0, 0x20027300, 0x0}},
      Title::Mode::All},
     {"Nintendo 3DS Camera",
      {{0x00020400, 0x00021400, 0x00022400, 0x00026400, 0x00027400, 0x00028400}},
      Title::Mode::All},
     {"Nintendo 3DS Sound",
      {{0x00020500, 0x00021500, 0x00022500, 0x00026500, 0x00027500, 0x00028500}},
      Title::Mode::All},
     {"Mii Maker",
      {{0x00020700, 0x00021700, 0x00022700, 0x00026700, 0x00027700, 0x00028700}},
      Title::Mode::Recommended},
     {"StreetPass Mii Plaza",
      {{0x00020800, 0x00021800, 0x00022800, 0x00026800, 0x00027800, 0x00028800}},
      Title::Mode::All},
     {"eShop",
      {{0x00020900, 0x00021900, 0x00022900, 0x0, 0x00027900, 0x00028900}},
      Title::Mode::Recommended},
     {"System Transfer",
      {{0x00020A00, 0x00021A00, 0x00022A00, 0x0, 0x00027A00, 0x00028A00}},
      Title::Mode::All},
     {"Nintendo Zone", {{0x00020B00, 0x00021B00, 0x00022B00, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"Face Raiders",
      {{0x00020D00, 0x00021D00, 0x00022D00, 0x00026D00, 0x00027D00, 0x00028D00}},
      Title::Mode::All},
     {"New_3DS Face Raiders",
      {{0x20020D00, 0x20021D00, 0x20022D00, 0x0, 0x20027D00, 0x0}},
      Title::Mode::All},
     {"AR Games",
      {{0x00020E00, 0x00021E00, 0x00022E00, 0x00026E00, 0x00027E00, 0x00028E00}},
      Title::Mode::All},
     {"Nintendo Network ID Settings",
      {{0x0002BF00, 0x0002C000, 0x0002C100, 0x0, 0x0, 0x0}},
      Title::Mode::Recommended},
     {"microSD Management",
      {{0x20023100, 0x20024100, 0x20025100, 0x0, 0x0, 0x0}},
      Title::Mode::All}}};

static const std::array<Title, 7> systemDataArchives = {
    {{"ClCertA",
      {{0x00010002, 0x00010002, 0x00010002, 0x00010002, 0x00010002, 0x00010002}},
      Title::Mode::Recommended},
     {"NS CFA",
      {{0x00010702, 0x00010702, 0x00010702, 0x00010702, 0x00010702, 0x00010702}},
      Title::Mode::All},
     {"dummy.txt",
      {{0x00010802, 0x00010802, 0x00010802, 0x00010802, 0x00010802, 0x00010802}},
      Title::Mode::All},
     {"CFA web-browser data",
      {{0x00018002, 0x00018002, 0x00018002, 0x00018002, 0x00018002, 0x00018002}},
      Title::Mode::All},
     {"local web-browser data",
      {{0x00018102, 0x00018102, 0x00018102, 0x00018102, 0x00018102, 0x00018102}},
      Title::Mode::All},
     {"webkit/OSS CROs",
      {{0x00018202, 0x00018202, 0x00018202, 0x00018202, 0x00018202, 0x00018202}},
      Title::Mode::All},
     {"Fangate_updater",
      {{0x00019002, 0x00019002, 0x00019002, 0x00019002, 0x00019002, 0x00019002}},
      Title::Mode::All}}};

static const std::array<Title, 27> systemApplets = {
    {{"Test Menu",
      {{0x00008102, 0x00008102, 0x00008102, 0x00008102, 0x00008102, 0x00008102}},
      Title::Mode::All},
     {"Home Menu",
      {{0x00008202, 0x00008F02, 0x00009802, 0x0000A102, 0x0000A902, 0x0000B102}},
      Title::Mode::All},
     {"Camera applet",
      {{0x00008402, 0x00009002, 0x00009902, 0x0000A202, 0x0000AA02, 0x0000B202}},
      Title::Mode::All},
     {"Instruction Manual",
      {{0x00008602, 0x00009202, 0x00009B02, 0x0000A402, 0x0000AC02, 0x0000B402}},
      Title::Mode::Recommended},
     {"Game Notes",
      {{0x00008702, 0x00009302, 0x00009C02, 0x0000A502, 0x0000AD02, 0x0000B502}},
      Title::Mode::All},
     {"Internet Browser",
      {{0x00008802, 0x00009402, 0x00009D02, 0x0000A602, 0x0000AE02, 0x0000B602}},
      Title::Mode::All},
     {"New 3DS Internet Browser",
      {{0x20008802, 0x20009402, 0x20009D02, 0x0, 0x2000AE02, 0x0}},
      Title::Mode::All},
     {"Fatal error viewer",
      {{0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02}},
      Title::Mode::All},
     {"Safe Mode Fatal error viewer",
      {{0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03}},
      Title::Mode::All},
     {"New 3DS Safe Mode Fatal error viewer",
      {{0x20008A03, 0x20008A03, 0x20008A03, 0x0, 0x20008A03, 0x0}},
      Title::Mode::All},
     {"Friend List",
      {{0x00008D02, 0x00009602, 0x00009F02, 0x0000A702, 0x0000AF02, 0x0000B702}},
      Title::Mode::Recommended},
     {"Notifications",
      {{0x00008E02, 0x000009702, 0x0000A002, 0x0000A802, 0x0000B002, 0x0000B802}},
      Title::Mode::All},
     {"Software Keyboard",
      {{0x00000C002, 0x0000C802, 0x0000D002, 0x0000D802, 0x0000DE02, 0x0000E402}},
      Title::Mode::Recommended},
     {"Safe Mode Software Keyboard",
      {{0x00000C003, 0x0000C803, 0x0000D003, 0x0000D803, 0x0000DE03, 0x0000E403}},
      Title::Mode::All},
     {"New 3DS Safe Mode Software Keyboard",
      {{0x2000C003, 0x2000C803, 0x2000D003, 0x0, 0x2000DE03, 0x0}},
      Title::Mode::All},
     {"Mii picker",
      {{0x0000C102, 0x0000C902, 0x0000D102, 0x0000D902, 0x0000DF02, 0x0000E502}},
      Title::Mode::Recommended},
     {"Picture picker",
      {{0x0000C302, 0x0000CB02, 0x0000D302, 0x0000DB02, 0x0000E102, 0x0000E702}},
      Title::Mode::All},
     {"Voice memo picker",
      {{0x0000C402, 0x0000CC02, 0x0000D402, 0x0000DC02, 0x0000E202, 0x0000E802}},
      Title::Mode::All},
     {"Error display",
      {{0x0000C502, 0x0000C502, 0x0000C502, 0x0000CF02, 0x0000CF02, 0x0000CF02}},
      Title::Mode::All},
     {"Safe mode error display",
      {{0x0000C503, 0x0000C503, 0x0000C503, 0x0000CF03, 0x0000CF03, 0x0000CF03}},
      Title::Mode::All},
     {"New 3DS safe mode error display",
      {{0x2000C503, 0x2000C503, 0x2000C503, 0x0, 0x2000CF03, 0x0}},
      Title::Mode::All},
     {"Circle Pad Pro test/calibration applet",
      {{0x0000CD02, 0x0000CD02, 0x0000CD02, 0x0000D502, 0x0000D502, 0x0000D502}},
      Title::Mode::All},
     {"eShop applet",
      {{0x0000C602, 0x0000CE02, 0x0000D602, 0x0, 0x0000E302, 0x0000E902}},
      Title::Mode::Recommended},
     {"Miiverse", {{0x0000BC02, 0x0000BC02, 0x0000BC02, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"Miiverse system library",
      {{0x0000F602, 0x0000F602, 0x0000F602, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Miiverse-posting applet",
      {{0x00008302, 0x00008B02, 0x0000BA02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Amiibo Settings",
      {{0x00009502, 0x00009E02, 0x0000B902, 0x0, 0x00008C02, 0x0000BF02}},
      Title::Mode::All}}};

static const std::array<Title, 25> sharedDataArchives = {
    {{"CFL_Res.dat",
      {{0x00010202, 0x00010202, 0x00010202, 0x00010202, 0x00010202, 0x00010202}},
      Title::Mode::All},
     {"Region Manifest",
      {{0x00010402, 0x00010402, 0x00010402, 0x00010402, 0x00010402, 0x00010402}},
      Title::Mode::All},
     {"Non-Nintendo TLS Root-CA Certificates",
      {{0x00010602, 0x00010602, 0x00010602, 0x00010602, 0x00010602, 0x00010602}},
      Title::Mode::Recommended},
     {"CHN/CN Dictionary", {{0x0, 0x0, 0x0, 0x00011002, 0x0, 0x0}}, Title::Mode::All},
     {"TWN/TN dictionary", {{0x0, 0x0, 0x0, 0x0, 0x0, 0x00011102}}, Title::Mode::All},
     {"NL/NL dictionary", {{0x0, 0x0, 0x00011202, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"EN/GB dictionary", {{0x0, 0x0, 0x00011302, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"EN/US dictionary", {{0x0, 0x00011402, 0x0, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"FR/FR/regular dictionary", {{0x0, 0x0, 0x00011502, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"FR/CA/regular dictionary", {{0x0, 0x00011602, 0x0, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"DE/regular dictionary", {{0x0, 0x0, 0x00011702, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"IT/IT dictionary", {{0x0, 0x0, 0x00011802, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"JA_small/32 dictionary", {{0x00011902, 0x0, 0x0, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"KO/KO dictionary", {{0x0, 0x0, 0x0, 0x0, 0x00011A02, 0x0}}, Title::Mode::All},
     {"PT/PT/regular dictionary", {{0x0, 0x0, 0x00011B02, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"RU/regular dictionary", {{0x0, 0x0, 0x00011C02, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"ES/ES dictionary", {{0x0, 0x00011D02, 0x00011D02, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"PT/BR/regular dictionary", {{0x0, 0x00011E02, 0x0, 0x0, 0x0, 0x0}}, Title::Mode::All},
     {"error strings",
      {{0x00012202, 0x00012302, 0x00012102, 0x00012402, 0x00012502, 0x00012602}},
      Title::Mode::All},
     {"eula", {{0x00013202, 0x00013302, 0x00013102, 0x00013502, 0x0, 0x0}}, Title::Mode::All},
     {"JPN/EUR/USA System Font",
      {{0x00014002, 0x00014002, 0x00014002, 0x00014002, 0x00014002, 0x00014002}},
      Title::Mode::Recommended},
     {"CHN System Font",
      {{0x00014102, 0x00014102, 0x00014102, 0x00014102, 0x00014102, 0x00014102}},
      Title::Mode::Recommended},
     {"KOR System Font",
      {{0x00014202, 0x00014202, 0x00014202, 0x00014202, 0x00014202, 0x00014202}},
      Title::Mode::Recommended},
     {"TWN System Font",
      {{0x00014302, 0x00014302, 0x00014302, 0x00014302, 0x00014302, 0x00014302}},
      Title::Mode::Recommended},
     {"rate",
      {{0x00015202, 0x00015302, 0x00015102, 0x0, 0x0015502, 0x00015602}},
      Title::Mode::All}}};

static const std::array<Title, 5> systemDataArchives2 = {
    {{"bad word list",
      {{0x00010302, 0x00010302, 0x00010302, 0x00010302, 0x00010302, 0x00010302}},
      Title::Mode::All},
     {"Nintendo Zone hotspot list",
      {{0x00010502, 0x00010502, 0x00010502, 0x00010502, 0x00010502, 0x00010502}},
      Title::Mode::All},
     {"NVer",
      {{0x00016102, 0x00016202, 0x00016302, 0x00016402, 0x00016502, 0x00016602}},
      Title::Mode::All},
     {"New_3DS NVer",
      {{0x20016102, 0x20016202, 0x20016302, 0x0, 0x20016502, 0x0}},
      Title::Mode::All},
     {"CVer",
      {{0x00017102, 0x00017202, 0x00017302, 0x00017402, 0x00017502, 0x00017602}},
      Title::Mode::All}}};

static const std::array<Title, 100> systemModules = {
    {{"sm",
      {{0x00001002, 0x00001002, 0x00001002, 0x00001002, 0x00001002, 0x00001002}},
      Title::Mode::All},
     {"Safe Mode sm",
      {{0x00001003, 0x00001003, 0x00001003, 0x00001003, 0x00001003, 0x00001003}},
      Title::Mode::All},
     {"fs",
      {{0x00001102, 0x00001102, 0x00001102, 0x00001102, 0x00001102, 0x00001102}},
      Title::Mode::All},
     {"Safe Mode fs",
      {{0x00001103, 0x00001103, 0x00001103, 0x00001103, 0x00001103, 0x00001103}},
      Title::Mode::All},
     {"pm",
      {{0x00001202, 0x00001202, 0x00001202, 0x00001202, 0x00001202, 0x00001202}},
      Title::Mode::All},
     {"Safe Mode pm",
      {{0x00001203, 0x00001203, 0x00001203, 0x00001203, 0x00001203, 0x00001203}},
      Title::Mode::All},
     {"loader",
      {{0x00001302, 0x00001302, 0x00001302, 0x00001302, 0x00001302, 0x00001302}},
      Title::Mode::All},
     {"Safe Mode loader",
      {{0x00001303, 0x00001303, 0x00001303, 0x00001303, 0x00001303, 0x00001303}},
      Title::Mode::All},
     {"pxi",
      {{0x00001402, 0x00001402, 0x00001402, 0x00001402, 0x00001402, 0x00001402}},
      Title::Mode::All},
     {"Safe Mode pxi",
      {{0x00001403, 0x00001403, 0x00001403, 0x00001403, 0x00001403, 0x00001403}},
      Title::Mode::All},
     {"AM ( Application Manager )",
      {{0x00001502, 0x00001502, 0x00001502, 0x00001502, 0x00001502, 0x00001502}},
      Title::Mode::All},
     {"Safe Mode AM",
      {{0x00001503, 0x00001503, 0x00001503, 0x00001503, 0x00001503, 0x00001503}},
      Title::Mode::All},
     {"New_3DS Safe Mode AM",
      {{0x20001503, 0x20001503, 0x20001503, 0x20001503, 0x20001503, 0x20001503}},
      Title::Mode::All},
     {"Camera",
      {{0x00001602, 0x00001602, 0x00001602, 0x00001602, 0x00001602, 0x00001602}},
      Title::Mode::All},
     {"New_3DS Camera",
      {{0x20001602, 0x20001602, 0x20001602, 0x20001602, 0x20001602, 0x20001602}},
      Title::Mode::All},
     {"Config (cfg)",
      {{0x00001702, 0x00001702, 0x00001702, 0x00001702, 0x00001702, 0x00001702}},
      Title::Mode::All},
     {"Safe Mode Config (cfg)",
      {{0x00001703, 0x00001703, 0x00001703, 0x00001703, 0x00001703, 0x00001703}},
      Title::Mode::All},
     {"New_3DS Safe Mode Config (cfg)",
      {{0x20001703, 0x20001703, 0x20001703, 0x20001703, 0x20001703, 0x20001703}},
      Title::Mode::All},
     {"Codec",
      {{0x00001802, 0x00001802, 0x00001802, 0x00001802, 0x00001802, 0x00001802}},
      Title::Mode::All},
     {"Safe Mode Codec",
      {{0x00001803, 0x00001803, 0x00001803, 0x00001803, 0x00001803, 0x00001803}},
      Title::Mode::All},
     {"New_3DS Safe Mode Codec",
      {{0x20001803, 0x20001803, 0x20001803, 0x20001803, 0x20001803, 0x20001803}},
      Title::Mode::All},
     {"DSP",
      {{0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02}},
      Title::Mode::All},
     {"Safe Mode DSP",
      {{0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03}},
      Title::Mode::All},
     {"New_3DS Safe Mode DSP",
      {{0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03}},
      Title::Mode::All},
     {"GPIO",
      {{0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02}},
      Title::Mode::All},
     {"Safe Mode GPIO",
      {{0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03}},
      Title::Mode::All},
     {"New_3DS Safe Mode GPIO",
      {{0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03}},
      Title::Mode::All},
     {"GSP",
      {{0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02}},
      Title::Mode::All},
     {"New_3DS GSP",
      {{0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02}},
      Title::Mode::All},
     {"Safe Mode GSP",
      {{0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03}},
      Title::Mode::All},
     {"New_3DS Safe Mode GSP",
      {{0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03}},
      Title::Mode::All},
     {"HID (Human Interface Devices)",
      {{0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02}},
      Title::Mode::All},
     {"Safe Mode HID",
      {{0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03}},
      Title::Mode::All},
     {"New_3DS Safe Mode HID",
      {{0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03}},
      Title::Mode::All},
     {"i2c",
      {{0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02}},
      Title::Mode::All},
     {"New_3DS i2c",
      {{0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02}},
      Title::Mode::All},
     {"Safe Mode i2c",
      {{0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03}},
      Title::Mode::All},
     {"New_3DS Safe Mode i2c",
      {{0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03}},
      Title::Mode::All},
     {"MCU",
      {{0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02}},
      Title::Mode::All},
     {"New_3DS MCU",
      {{0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02}},
      Title::Mode::All},
     {"Safe Mode MCU",
      {{0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03}},
      Title::Mode::All},
     {"New_3DS Safe Mode MCU",
      {{0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03}},
      Title::Mode::All},
     {"MIC (Microphone)",
      {{0x00002002, 0x00002002, 0x00002002, 0x00002002, 0x00002002, 0x00002002}},
      Title::Mode::All},
     {"PDN",
      {{0x00002102, 0x00002102, 0x00002102, 0x00002102, 0x00002102, 0x00002102}},
      Title::Mode::All},
     {"Safe Mode PDN",
      {{0x00002103, 0x00002103, 0x00002103, 0x00002103, 0x00002103, 0x00002103}},
      Title::Mode::All},
     {"New_3DS Safe Mode PDN",
      {{0x20002103, 0x20002103, 0x20002103, 0x20002103, 0x20002103, 0x20002103}},
      Title::Mode::All},
     {"PTM (Play time, pedometer, and battery manager)",
      {{0x00002202, 0x00002202, 0x00002202, 0x00002202, 0x00002202, 0x00002202}},
      Title::Mode::All},
     {"New_3DS PTM (Play time, pedometer, and battery manager)",
      {{0x20002202, 0x20002202, 0x20002202, 0x20002202, 0x20002202, 0x20002202}},
      Title::Mode::All},
     {"Safe Mode PTM",
      {{0x00002203, 0x00002203, 0x00002203, 0x00002203, 0x00002203, 0x00002203}},
      Title::Mode::All},
     {"New_3DS Safe Mode PTM",
      {{0x20002203, 0x20002203, 0x20002203, 0x20002203, 0x20002203, 0x20002203}},
      Title::Mode::All},
     {"spi",
      {{0x00002302, 0x00002302, 0x00002302, 0x00002302, 0x00002302, 0x00002302}},
      Title::Mode::All},
     {"New_3DS spi",
      {{0x20002302, 0x20002302, 0x20002302, 0x20002302, 0x20002302, 0x20002302}},
      Title::Mode::All},
     {"Safe Mode spi",
      {{0x00002303, 0x00002303, 0x00002303, 0x00002303, 0x00002303, 0x00002303}},
      Title::Mode::All},
     {"New_3DS Safe Mode spi",
      {{0x20002303, 0x20002303, 0x20002303, 0x20002303, 0x20002303, 0x20002303}},
      Title::Mode::All},
     {"AC (Network manager)",
      {{0x00002402, 0x00002402, 0x00002402, 0x00002402, 0x00002402, 0x00002402}},
      Title::Mode::All},
     {"Safe Mode AC",
      {{0x00002403, 0x00002403, 0x00002403, 0x00002403, 0x00002403, 0x00002403}},
      Title::Mode::All},
     {"New_3DS Safe Mode AC",
      {{0x20002403, 0x20002403, 0x20002403, 0x20002403, 0x20002403, 0x20002403}},
      Title::Mode::All},
     {"Cecd (StreetPass)",
      {{0x00002602, 0x00002602, 0x00002602, 0x00002602, 0x00002602, 0x00002602}},
      Title::Mode::All},
     {"CSND",
      {{0x00002702, 0x00002702, 0x00002702, 0x00002702, 0x00002702, 0x00002702}},
      Title::Mode::All},
     {"Safe Mode CSND",
      {{0x00002703, 0x00002703, 0x00002703, 0x00002703, 0x00002703, 0x00002703}},
      Title::Mode::All},
     {"New_3DS Safe Mode CSND",
      {{0x20002703, 0x20002703, 0x20002703, 0x20002703, 0x20002703, 0x20002703}},
      Title::Mode::All},
     {"DLP (Download Play)",
      {{0x00002802, 0x00002802, 0x00002802, 0x00002802, 0x00002802, 0x00002802}},
      Title::Mode::Recommended},
     {"HTTP",
      {{0x00002902, 0x00002902, 0x00002902, 0x00002902, 0x00002902, 0x00002902}},
      Title::Mode::All},
     {"Safe Mode HTTP",
      {{0x00002903, 0x00002903, 0x00002903, 0x00002903, 0x00002903, 0x00002903}},
      Title::Mode::All},
     {"New_3DS Safe Mode HTTP",
      {{0x20002903, 0x20002903, 0x20002903, 0x20002903, 0x20002903, 0x20002903}},
      Title::Mode::All},
     {"MP",
      {{0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02}},
      Title::Mode::All},
     {"Safe Mode MP",
      {{0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03}},
      Title::Mode::All},
     {"NDM",
      {{0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02}},
      Title::Mode::All},
     {"NIM",
      {{0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02}},
      Title::Mode::All},
     {"Safe Mode NIM",
      {{0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03}},
      Title::Mode::All},
     {"New_3DS Safe Mode NIM",
      {{0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03}},
      Title::Mode::All},
     {"NWM ( Low-level wifi manager )",
      {{0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02}},
      Title::Mode::All},
     {"Safe Mode NWM",
      {{0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03}},
      Title::Mode::All},
     {"New_3DS Safe Mode NWM",
      {{0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03}},
      Title::Mode::All},
     {"Sockets",
      {{0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02}},
      Title::Mode::All},
     {"Safe Mode Sockets",
      {{0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03}},
      Title::Mode::All},
     {"New_3DS Safe Mode Sockets",
      {{0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03}},
      Title::Mode::All},
     {"SSL",
      {{0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02}},
      Title::Mode::All},
     {"Safe Mode SSL",
      {{0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03}},
      Title::Mode::All},
     {"New_3DS Safe Mode SSL",
      {{0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03}},
      Title::Mode::All},
     {"Process9",
      {{0x00003000, 0x00003000, 0x00003000, 0x00003000, 0x00003000, 0x00003000}},
      Title::Mode::All},
     {"PS ( Process Manager )",
      {{0x00003102, 0x00003102, 0x00003102, 0x00003102, 0x00003102, 0x00003102}},
      Title::Mode::All},
     {"Safe Mode PS",
      {{0x00003103, 0x00003103, 0x00003103, 0x00003103, 0x00003103, 0x00003103}},
      Title::Mode::All},
     {"New_3DS Safe Mode PS",
      {{0x20003103, 0x20003103, 0x20003103, 0x20003103, 0x20003103, 0x20003103}},
      Title::Mode::All},
     {"friends (Friends list)",
      {{0x00003202, 0x00003202, 0x00003202, 0x00003202, 0x00003202, 0x00003202}},
      Title::Mode::All},
     {"Safe Mode friends (Friends list)",
      {{0x00003203, 0x00003203, 0x00003203, 0x00003203, 0x00003203, 0x00003203}},
      Title::Mode::All},
     {"New_3DS Safe Mode friends (Friends list)",
      {{0x20003203, 0x20003203, 0x20003203, 0x20003203, 0x20003203, 0x20003203}},
      Title::Mode::All},
     {"IR (Infrared)",
      {{0x00003302, 0x00003302, 0x00003302, 0x00003302, 0x00003302, 0x00003302}},
      Title::Mode::All},
     {"Safe Mode IR",
      {{0x00003303, 0x00003303, 0x00003303, 0x00003303, 0x00003303, 0x00003303}},
      Title::Mode::All},
     {"New_3DS Safe Mode IR",
      {{0x20003303, 0x20003303, 0x20003303, 0x20003303, 0x20003303, 0x20003303}},
      Title::Mode::All},
     {"BOSS (SpotPass)",
      {{0x00003402, 0x00003402, 0x00003402, 0x00003402, 0x00003402, 0x00003402}},
      Title::Mode::All},
     {"News (Notifications)",
      {{0x00003502, 0x00003502, 0x00003502, 0x00003502, 0x00003502, 0x00003502}},
      Title::Mode::All},
     {"RO",
      {{0x00003702, 0x00003702, 0x00003702, 0x00003702, 0x00003702, 0x00003702}},
      Title::Mode::All},
     {"act",
      {{0x00003802, 0x00003802, 0x00003802, 0x00003802, 0x00003802, 0x00003802}},
      Title::Mode::All},
     {"nfc",
      {{0x00004002, 0x00004002, 0x00004002, 0x00004002, 0x00004002, 0x00004002}},
      Title::Mode::All},
     {"New_3DS mvd",
      {{0x20004102, 0x20004102, 0x20004102, 0x20004102, 0x20004102, 0x20004102}},
      Title::Mode::All},
     {"New_3DS qtm",
      {{0x20004202, 0x20004202, 0x20004202, 0x20004202, 0x20004202, 0x20004202}},
      Title::Mode::All},
     {"NS",
      {{0x00008002, 0x00008002, 0x00008002, 0x00008002, 0x00008002, 0x00008002}},
      Title::Mode::All},
     {"Safe Mode NS",
      {{0x00008003, 0x00008003, 0x00008003, 0x00008003, 0x00008003, 0x00008003}},
      Title::Mode::All},
     {"New_3DS Safe Mode NS",
      {{0x20008003, 0x20008003, 0x20008003, 0x20008003, 0x20008003, 0x20008003}},
      Title::Mode::All}}};

ConfigureSystem::ConfigureSystem(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureSystem) {
    ui->setupUi(this);
    connect(ui->combo_birthmonth,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureSystem::UpdateBirthdayComboBox);
    connect(ui->combo_init_clock,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureSystem::UpdateInitTime);
    connect(ui->button_regenerate_console_id, &QPushButton::clicked, this,
            &ConfigureSystem::RefreshConsoleID);
    connect(ui->button_start_download, &QPushButton::clicked, this,
            &ConfigureSystem::DownloadFromNUS);
    for (u8 i = 0; i < country_names.size(); i++) {
        if (std::strcmp(country_names.at(i), "") != 0) {
            ui->combo_country->addItem(tr(country_names.at(i)), i);
        }
    }

    ui->combo_download_mode->setCurrentIndex(1); // set to Recommended
    bool keysAvailable = true;
    HW::AES::InitKeys(true);
    for (std::size_t i = 0; i < HW::AES::MaxCommonKeySlot; i++) {
        HW::AES::SelectCommonKeyIndex(i);
        if (!HW::AES::IsNormalKeyAvailable(HW::AES::KeySlotID::TicketCommonKey)) {
            keysAvailable = false;
            break;
        }
    }
    if (!keysAvailable) {
        ui->button_start_download->setEnabled(false);
        ui->combo_download_mode->setEnabled(false);
        ui->label_nus_download->setText(
            tr("Citra is missing keys to download system files. <br><a "
               "href='https://citra-emu.org/wiki/aes-keys/'><span style=\"text-decoration: "
               "underline; color:#039be5;\">How to get keys?</span></a>"));
    } else {
        ui->button_start_download->setEnabled(true);
        ui->combo_download_mode->setEnabled(true);
        ui->label_nus_download->setText(tr("Download System Files from Nitendo servers"));
    }

    ConfigureTime();
}

ConfigureSystem::~ConfigureSystem() = default;

void ConfigureSystem::SetConfiguration() {
    enabled = !Core::System::GetInstance().IsPoweredOn();

    ui->combo_init_clock->setCurrentIndex(static_cast<u8>(Settings::values.init_clock));
    QDateTime date_time;
    date_time.setTime_t(Settings::values.init_time);
    ui->edit_init_time->setDateTime(date_time);

    if (!enabled) {
        cfg = Service::CFG::GetModule(Core::System::GetInstance());
        ASSERT_MSG(cfg, "CFG Module missing!");
        ReadSystemSettings();
        ui->group_system_settings->setEnabled(false);
    } else {
        // This tab is enabled only when game is not running (i.e. all service are not initialized).
        cfg = std::make_shared<Service::CFG::Module>();
        ReadSystemSettings();

        ui->label_disable_info->hide();
    }
}

void ConfigureSystem::ReadSystemSettings() {
    // set username
    username = cfg->GetUsername();
    // TODO(wwylele): Use this when we move to Qt 5.5
    // ui->edit_username->setText(QString::fromStdU16String(username));
    ui->edit_username->setText(
        QString::fromUtf16(reinterpret_cast<const ushort*>(username.data())));

    // set birthday
    std::tie(birthmonth, birthday) = cfg->GetBirthday();
    ui->combo_birthmonth->setCurrentIndex(birthmonth - 1);
    UpdateBirthdayComboBox(
        birthmonth -
        1); // explicitly update it because the signal from setCurrentIndex is not reliable
    ui->combo_birthday->setCurrentIndex(birthday - 1);

    // set system language
    language_index = cfg->GetSystemLanguage();
    ui->combo_language->setCurrentIndex(language_index);

    // set sound output mode
    sound_index = cfg->GetSoundOutputMode();
    ui->combo_sound->setCurrentIndex(sound_index);

    // set the country code
    country_code = cfg->GetCountryCode();
    ui->combo_country->setCurrentIndex(ui->combo_country->findData(country_code));

    // set the console id
    u64 console_id = cfg->GetConsoleUniqueId();
    ui->label_console_id->setText(
        tr("Console ID: 0x%1").arg(QString::number(console_id, 16).toUpper()));

    // set play coin
    play_coin = Service::PTM::Module::GetPlayCoins();
    ui->spinBox_play_coins->setValue(play_coin);
}

void ConfigureSystem::ApplyConfiguration() {
    if (!enabled) {
        return;
    }

    bool modified = false;

    // apply username
    // TODO(wwylele): Use this when we move to Qt 5.5
    // std::u16string new_username = ui->edit_username->text().toStdU16String();
    std::u16string new_username(
        reinterpret_cast<const char16_t*>(ui->edit_username->text().utf16()));
    if (new_username != username) {
        cfg->SetUsername(new_username);
        modified = true;
    }

    // apply birthday
    int new_birthmonth = ui->combo_birthmonth->currentIndex() + 1;
    int new_birthday = ui->combo_birthday->currentIndex() + 1;
    if (birthmonth != new_birthmonth || birthday != new_birthday) {
        cfg->SetBirthday(new_birthmonth, new_birthday);
        modified = true;
    }

    // apply language
    int new_language = ui->combo_language->currentIndex();
    if (language_index != new_language) {
        cfg->SetSystemLanguage(static_cast<Service::CFG::SystemLanguage>(new_language));
        modified = true;
    }

    // apply sound
    int new_sound = ui->combo_sound->currentIndex();
    if (sound_index != new_sound) {
        cfg->SetSoundOutputMode(static_cast<Service::CFG::SoundOutputMode>(new_sound));
        modified = true;
    }

    // apply country
    u8 new_country = static_cast<u8>(ui->combo_country->currentData().toInt());
    if (country_code != new_country) {
        cfg->SetCountryCode(new_country);
        modified = true;
    }

    // apply play coin
    u16 new_play_coin = static_cast<u16>(ui->spinBox_play_coins->value());
    if (play_coin != new_play_coin) {
        Service::PTM::Module::SetPlayCoins(new_play_coin);
    }

    // update the config savegame if any item is modified.
    if (modified) {
        cfg->UpdateConfigNANDSavegame();
    }

    Settings::values.init_clock =
        static_cast<Settings::InitClock>(ui->combo_init_clock->currentIndex());
    Settings::values.init_time = ui->edit_init_time->dateTime().toTime_t();
    Settings::Apply();
}

void ConfigureSystem::UpdateBirthdayComboBox(int birthmonth_index) {
    if (birthmonth_index < 0 || birthmonth_index >= 12)
        return;

    // store current day selection
    int birthday_index = ui->combo_birthday->currentIndex();

    // get number of days in the new selected month
    int days = days_in_month[birthmonth_index];

    // if the selected day is out of range,
    // reset it to 1st
    if (birthday_index < 0 || birthday_index >= days)
        birthday_index = 0;

    // update the day combo box
    ui->combo_birthday->clear();
    for (int i = 1; i <= days; ++i) {
        ui->combo_birthday->addItem(QString::number(i));
    }

    // restore the day selection
    ui->combo_birthday->setCurrentIndex(birthday_index);
}

void ConfigureSystem::ConfigureTime() {
    ui->edit_init_time->setCalendarPopup(true);
    QDateTime dt;
    dt.fromString(QStringLiteral("2000-01-01 00:00:01"), QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    ui->edit_init_time->setMinimumDateTime(dt);

    SetConfiguration();

    UpdateInitTime(ui->combo_init_clock->currentIndex());
}

void ConfigureSystem::UpdateInitTime(int init_clock) {
    const bool is_fixed_time =
        static_cast<Settings::InitClock>(init_clock) == Settings::InitClock::FixedTime;
    ui->label_init_time->setVisible(is_fixed_time);
    ui->edit_init_time->setVisible(is_fixed_time);
}

void ConfigureSystem::RefreshConsoleID() {
    QMessageBox::StandardButton reply;
    QString warning_text = tr("This will replace your current virtual 3DS with a new one. "
                              "Your current virtual 3DS will not be recoverable. "
                              "This might have unexpected effects in games. This might fail, "
                              "if you use an outdated config savegame. Continue?");
    reply = QMessageBox::critical(this, tr("Warning"), warning_text,
                                  QMessageBox::No | QMessageBox::Yes);
    if (reply == QMessageBox::No) {
        return;
    }

    u32 random_number;
    u64 console_id;
    cfg->GenerateConsoleUniqueId(random_number, console_id);
    cfg->SetConsoleUniqueId(random_number, console_id);
    cfg->UpdateConfigNANDSavegame();
    ui->label_console_id->setText(
        tr("Console ID: 0x%1").arg(QString::number(console_id, 16).toUpper()));
}

void ConfigureSystem::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureSystem::DownloadFromNUS() {
    int mode = ui->combo_download_mode->currentIndex();
    u32 region_value = cfg->GetRegionValue();
    if (region_value == 0) {
        region_value = 2;
    } else {
        --region_value;
    }

    auto TitlesWithMode = [mode, region_value](const Title& title) {
        return mode <= title.mode && title.lower_title_id[region_value] != 0;
    };
    std::size_t numTitles =
        std::count_if(systemFirmware.begin(), systemFirmware.end(), TitlesWithMode);
    numTitles +=
        std::count_if(systemApplications.begin(), systemApplications.end(), TitlesWithMode);
    numTitles +=
        std::count_if(systemDataArchives.begin(), systemDataArchives.end(), TitlesWithMode);
    numTitles += std::count_if(systemApplets.begin(), systemApplets.end(), TitlesWithMode);
    numTitles +=
        std::count_if(sharedDataArchives.begin(), sharedDataArchives.end(), TitlesWithMode);
    numTitles +=
        std::count_if(systemDataArchives2.begin(), systemDataArchives2.end(), TitlesWithMode);
    numTitles += std::count_if(systemModules.begin(), systemModules.end(), TitlesWithMode);

    QProgressDialog progress(tr("Downloading files..."), tr("Abort"), 0, numTitles, this);
    progress.setWindowModality(Qt::WindowModal);

    u32 upperTitleID;
    std::size_t count = 0;
    bool failed = false;
    auto InstallIfMode = [mode, region_value, &progress, &upperTitleID, &count,
                          &failed](const Title& title, int version = -1) {
        if (progress.wasCanceled())
            return;
        if (mode <= title.mode && title.lower_title_id[region_value] != 0) {
            progress.setValue(count++);
            u64 title_id = (static_cast<u64>(upperTitleID) << 32) +
                           static_cast<u64>(title.lower_title_id[region_value]);
            LOG_DEBUG(Service_AM, "Downloading {:X}", title_id);
            if (Service::AM::InstallFromNus(title_id, version) !=
                Service::AM::InstallStatus::Success) {
                failed = true;
            }
        }
    };

    // Install save mode firms with v1.0 first
    if (!progress.wasCanceled()) {
        upperTitleID = 0x00040138;
        constexpr int SAFE_MODE_NATIVE_FIRM_V1_VERSION = 432;
        InstallIfMode(systemFirmware[0], SAFE_MODE_NATIVE_FIRM_V1_VERSION);
        HW::AES::InitKeys(true);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x00040138;
        std::for_each(systemFirmware.begin() + 1, systemFirmware.end(), InstallIfMode);
        HW::AES::InitKeys(true);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x00040010;
        std::for_each(systemApplications.begin(), systemApplications.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x0004001B;
        std::for_each(systemDataArchives.begin(), systemDataArchives.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x00040030;
        std::for_each(systemApplets.begin(), systemApplets.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x0004009B;
        std::for_each(sharedDataArchives.begin(), sharedDataArchives.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x000400DB;
        std::for_each(systemDataArchives2.begin(), systemDataArchives2.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        upperTitleID = 0x00040130;
        std::for_each(systemModules.begin(), systemModules.end(), InstallIfMode);
    }
    if (!progress.wasCanceled()) {
        progress.setValue(count);
        progress.cancel();
    }
    if (failed) {
        QMessageBox msgBox;
        msgBox.setText(tr("Downloading system files failed"));
        msgBox.exec();
    }
}
