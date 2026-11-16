#include "i18n.h"
#include <string.h>
#include <strings.h>

static const char* const kEn[] = {
#define X(k, en, pt) en,
    TR_LIST(X)
#undef X
};
static const char* const kPt[] = {
#define X(k, en, pt) pt,
    TR_LIST(X)
#undef X
};

static Lang g_lang = Lang::EN;

void i18nSetLang(Lang lang) { g_lang = lang; }

const char* tr(Tr key) {
  if (key >= Tr::Count) return "";
  return (g_lang == Lang::PT_BR ? kPt : kEn)[static_cast<uint16_t>(key)];
}

Lang langFromCode(const char* code) {
  if (code && (!strcasecmp(code, "pt-br") || !strcasecmp(code, "pt_br"))) return Lang::PT_BR;
  return Lang::EN;
}
