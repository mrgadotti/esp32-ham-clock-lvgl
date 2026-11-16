#include <unity.h>
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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_tr_default_en);
  RUN_TEST(test_tr_pt);
  RUN_TEST(test_lang_from_code);
  RUN_TEST(test_every_key_has_both_langs);
  return UNITY_END();
}
