#include <unity.h>
#include <string.h>
#include "propagation_xml.h"
void setUp() {} void tearDown() {}

void test_prop_fixture() {
  const char* xml =
    "<solar><solardata><updated> 16 Nov 2026 1200 GMT </updated><solarflux>150</solarflux>"
    "<aindex>8</aindex><kindex>3</kindex><muf>21.3</muf><calculatedconditions>"
    "<band name=\"80m-40m\" time=\"day\">Fair</band><band name=\"80m-40m\" time=\"night\">Good</band>"
    "<band name=\"12m-10m\" time=\"day\">Poor</band></calculatedconditions></solardata></solar>";
  PropagationData p; memset(&p, 0, sizeof p);
  TEST_ASSERT_TRUE(parsePropagationXml(xml, p));
  TEST_ASSERT_TRUE(p.valid);
  TEST_ASSERT_EQUAL_STRING("150", p.sfi); TEST_ASSERT_EQUAL_STRING("3", p.kindex);
  TEST_ASSERT_EQUAL_STRING("8", p.aindex); TEST_ASSERT_EQUAL_STRING("21.3", p.muf);
  TEST_ASSERT_EQUAL_STRING("16 Nov 2026 1200 GMT", p.updated);
  TEST_ASSERT_EQUAL_STRING("80m-40m", p.band[0].label);
  TEST_ASSERT_EQUAL_STRING("Fair", p.band[0].day); TEST_ASSERT_EQUAL_STRING("Good", p.band[0].night);
  TEST_ASSERT_EQUAL_STRING("--", p.band[1].day); TEST_ASSERT_EQUAL_STRING("30m-20m", p.band[1].label);
  TEST_ASSERT_EQUAL_STRING("Poor", p.band[3].day); TEST_ASSERT_EQUAL_STRING("--", p.band[3].night);
}

void test_prop_rejects_garbage() {
  PropagationData p; memset(&p, 0, sizeof p);
  strcpy(p.sfi, "99"); p.valid = true;
  TEST_ASSERT_FALSE(parsePropagationXml("<html><body>503 Service Unavailable</body></html>", p));
  TEST_ASSERT_TRUE(p.valid); TEST_ASSERT_EQUAL_STRING("99", p.sfi);
}

int main() { UNITY_BEGIN(); RUN_TEST(test_prop_fixture); RUN_TEST(test_prop_rejects_garbage); return UNITY_END(); }
