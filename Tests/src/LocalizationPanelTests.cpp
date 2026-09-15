#include <kimia_test.h>
#include <kimia/LocalizationPanel.h>

KIMIA_TEST(Locale_DrawEmptyDoesNotCrash) {
  kimia::ui::drawLocalizationPanel({0, 0, 280, 200}, {}, 0);
}

KIMIA_TEST(Locale_DrawOneLocale) {
  std::vector<kimia::ui::LocaleEntry> v(1);
  v[0].code = "fa-IR";
  v[0].displayName = "فارسی";
  v[0].stringCount = 100;
  v[0].translatedCount = 100;
  v[0].isCurrent = true;
  kimia::ui::drawLocalizationPanel({0, 0, 280, 200}, v, 0);
}

KIMIA_TEST(Locale_DrawManyLocales) {
  std::vector<kimia::ui::LocaleEntry> v;
  const char* codes[] = {"fa-IR", "en-US", "fr-FR", "de-DE", "es-ES", "ja-JP"};
  const char* names[] = {"فارسی", "English", "Français",
                        "Deutsch", "Español", "日本語"};
  for (int i = 0; i < 6; ++i) {
    kimia::ui::LocaleEntry l;
    l.code = codes[i];
    l.displayName = names[i];
    l.stringCount = 100;
    l.translatedCount = 50 + i * 8;
    l.isCurrent = (i == 0);
    v.push_back(l);
  }
  kimia::ui::drawLocalizationPanel({0, 0, 320, 200}, v, 0);
}

KIMIA_TEST(Locale_DrawWithPartialTranslation) {
  std::vector<kimia::ui::LocaleEntry> v(1);
  v[0].code = "ru-RU";
  v[0].displayName = "Русский";
  v[0].stringCount = 200;
  v[0].translatedCount = 50;
  kimia::ui::drawLocalizationPanel({0, 0, 280, 100}, v, 0);
}

KIMIA_TEST(Locale_DrawAtScroll) {
  std::vector<kimia::ui::LocaleEntry> v;
  for (int i = 0; i < 20; ++i) {
    kimia::ui::LocaleEntry l;
    l.code = "x" + std::to_string(i);
    l.displayName = "l_" + std::to_string(i);
    l.stringCount = 100;
    l.translatedCount = i * 5;
    v.push_back(l);
  }
  kimia::ui::drawLocalizationPanel({0, 0, 280, 200}, v, -50);
  kimia::ui::drawLocalizationPanel({0, 0, 280, 200}, v, 100);
}

KIMIA_TEST(Locale_DrawAtPhonePortrait) {
  std::vector<kimia::ui::LocaleEntry> v;
  for (int i = 0; i < 5; ++i) {
    kimia::ui::LocaleEntry l;
    l.code = "x";
    l.displayName = "L" + std::to_string(i);
    v.push_back(l);
  }
  kimia::ui::drawLocalizationPanel({0, 0, 240, 320}, v, 0);
}

KIMIA_TEST(Locale_DrawAtTabletLandscape) {
  std::vector<kimia::ui::LocaleEntry> v;
  for (int i = 0; i < 15; ++i) {
    kimia::ui::LocaleEntry l;
    l.code = "x" + std::to_string(i);
    l.displayName = "Locale " + std::to_string(i);
    l.stringCount = 100;
    l.translatedCount = i * 5;
    l.isCurrent = (i == 3);
    v.push_back(l);
  }
  kimia::ui::drawLocalizationPanel({0, 0, 480, 320}, v, 0);
}
