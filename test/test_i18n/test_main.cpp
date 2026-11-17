#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>
#include "i18n.h"

void setUp() { i18nSetLang(Lang::EN); }
void tearDown() {}

void test_tr_default_en() { TEST_ASSERT_EQUAL_STRING("WEATHER", tr(Tr::Weather)); }
void test_tr_pt() {
  i18nSetLang(Lang::PT_BR);
  TEST_ASSERT_EQUAL_STRING("CLIMA", tr(Tr::Weather));
}
void test_lang_from_code() {
  TEST_ASSERT_EQUAL((int)Lang::PT_BR, (int)langFromCode("pt-br"));
  TEST_ASSERT_EQUAL((int)Lang::PT_BR, (int)langFromCode("pt_br"));
  TEST_ASSERT_EQUAL((int)Lang::EN, (int)langFromCode("xx"));
  TEST_ASSERT_EQUAL((int)Lang::EN, (int)langFromCode(nullptr));
}
void test_every_key_has_both_langs() {
  for (Lang l : {Lang::EN, Lang::PT_BR}) {
    i18nSetLang(l);
    for (uint16_t i = 0; i < (uint16_t)Tr::Count; i++) {
      const char* s = tr((Tr)i);
      TEST_ASSERT_NOT_NULL(s);
      TEST_ASSERT_TRUE(strlen(s) > 0);
    }
  }
}

// Mojibake detector: no C1 control (U+0080-U+009F = C2 80..9F) and no "Ã" (C3 83) followed by a continuation-looking C2 xx.
void test_no_mojibake() {
  for (Lang l : {Lang::EN, Lang::PT_BR}) {
    i18nSetLang(l);
    for (uint16_t i = 0; i < (uint16_t)Tr::Count; i++) {
      const unsigned char* s = (const unsigned char*)tr((Tr)i);
      for (; *s; s++) {
        char msg[24];
        snprintf(msg, sizeof msg, "key %u lang %d", (unsigned)i, (int)l);
        if (s[0] == 0xC2) TEST_ASSERT_FALSE_MESSAGE(s[1] >= 0x80 && s[1] <= 0x9F, msg);
        if (s[0] == 0xC3 && s[1] == 0x83) TEST_ASSERT_FALSE_MESSAGE(s[2] == 0xC2, msg);
      }
    }
  }
}
void test_times_sign() {
  TEST_ASSERT_EQUAL_STRING("1\xC3\x97", tr(Tr::RadarZoomReset));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_tr_default_en);
  RUN_TEST(test_tr_pt);
  RUN_TEST(test_lang_from_code);
  RUN_TEST(test_every_key_has_both_langs);
  RUN_TEST(test_no_mojibake);
  RUN_TEST(test_times_sign);
  return UNITY_END();
}
