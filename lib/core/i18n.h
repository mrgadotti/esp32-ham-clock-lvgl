#pragma once
#include <stdint.h>

enum class Lang : uint8_t { EN = 0, PT_BR = 1 };

// X(key, English, Brazilian Portuguese). Single source for the enum and both tables.
#define TR_LIST(X) \
  /* shared */ \
  X(Utc, "UTC", "UTC") \
  X(LocalTime, "Local Time", "Hora Local") \
  X(Local, "LOCAL", "LOCAL") \
  X(Close, "CLOSE", "FECHAR") \
  X(Save, "SAVE", "SALVAR") \
  X(Default, "DEFAULT", "PADRÃO") \
  X(Ok, "OK", "OK") \
  X(Yes, "YES", "SIM") \
  X(No, "NO", "NÃO") \
  X(All, "ALL", "TODOS") \
  X(Mode, "MODE", "MODO") \
  X(Band, "BAND", "BANDA") \
  X(NoData, "No data", "Sem dados") \
  X(ErrorPrefix, "ERROR: ", "ERRO: ") \
  X(Page, "Page", "Página") \
  X(SwitchScreen, "SWITCH SCREEN", "TROCAR TELA") \
  X(WaitingForData, "WAITING FOR DATA...", "AGUARDANDO DADOS...") \
  X(UpdatedUtc, "UPDATED UTC", "ATUALIZADO UTC") \
  X(NoConnection, "NO CONNECTION", "SEM CONEXÃO") \
  /* clock */ \
  X(Clock, "HAM CLOCK", "RELÓGIO HAM") \
  X(OperatorStation, "OPERATOR STATION", "ESTAÇÃO DO OPERADOR") \
  X(TimeMode, "TIME MODE", "MODO DE HORA") \
  X(Gps, "GPS", "GPS") \
  /* table columns */ \
  X(ColUtc, "UTC", "UTC") \
  X(ColCall, "CALL", "INDICATIVO") \
  X(ColCallsign, "CALLSIGN", "INDICATIVO") \
  X(ColMhz, "MHz", "MHz") \
  X(ColMode, "MODE", "MODO") \
  X(ColMsg, "MSG", "MSG") \
  X(ColCountry, "COUNTRY", "PAÍS") \
  X(ColSymbol, "SYMBOL", "SÍMBOLO") \
  X(ColKm, "KM", "KM") \
  X(ColFreq, "FREQ", "FREQ") \
  X(ColBand, "BAND", "BANDA") \
  X(ColDay, "DAY", "DIA") \
  X(ColNight, "NIGHT", "NOITE") \
  /* DX cluster / POTA / HamAlert */ \
  X(DxCluster, "DX CLUSTER", "DX CLUSTER") \
  X(Pota, "POTA.app", "POTA.app") \
  X(HamAlert, "HAMALERT.org", "HAMALERT.org") \
  X(WaitingForSpots, "WAITING FOR SPOTS...", "AGUARDANDO SPOTS...") \
  X(Filters, "FILTERS", "FILTROS") \
  X(PotaFilters, "POTA FILTERS", "FILTROS POTA") \
  X(HamAlertFilters, "HAMALERT FILTERS", "FILTROS HAMALERT") \
  X(FilterCw, "CW", "CW") \
  X(FilterSsb, "SSB", "SSB") \
  X(FilterDigi, "DIGI", "DIGI") \
  /* APRS */ \
  X(AprsIs, "APRS-IS", "APRS-IS") \
  X(AprsKiss, "APRS-TNC-KISS", "APRS-TNC-KISS") \
  X(AprsTnc2, "APRS-TNC2-UART", "APRS-TNC2-UART") \
  X(AprsRadar, "APRS RADAR", "RADAR APRS") \
  X(WaitingForAprs, "WAITING FOR APRS...", "AGUARDANDO APRS...") \
  X(SortBy, "SORT BY:", "ORDENAR POR:") \
  X(SortTime, "TIME", "HORA") \
  X(SortCall, "CALL", "INDICATIVO") \
  X(SortDist, "DIST", "DIST") \
  X(AprsSetting, "APRS Setting:", "Config. APRS:") \
  X(Beaconing, "Beaconing", "Beacon") \
  X(AprsAlert, "APRS Alert", "Alerta APRS") \
  X(RangeAlert, "Range Alert", "Alerta de alcance") \
  X(LedBuzzer, "LED / Buzzer", "LED / Buzzer") \
  X(ZoomIn, "ZOOM +", "ZOOM +") \
  X(ZoomOut, "ZOOM -", "ZOOM -") \
  X(Zoom, "ZOOM:", "ZOOM:") \
  X(RadarRmax, "Rmax:", "Rmáx:") \
  X(RadarAlert, "ALERT:", "ALERTA:") \
  /* propagation / HF bands */ \
  X(Propagation, "PROPAGATION", "PROPAGAÇÃO") \
  X(HfBands, "HF BAND INFO", "BANDAS HF") \
  X(Sfi, "SFI", "SFI") \
  X(KIndex, "K-INDEX", "ÍNDICE K") \
  X(AIndex, "A-INDEX", "ÍNDICE A") \
  X(Muf, "MUF", "MUF") \
  X(Good, "Good", "Boa") \
  X(Fair, "Fair", "Regular") \
  X(Poor, "Poor", "Ruim") \
  /* weather */ \
  X(Weather, "WEATHER", "CLIMA") \
  X(Forecast, "Weather forecast", "Previsão do tempo") \
  X(Temperature, "TEMPERATURE", "TEMPERATURA") \
  X(Humidity, "HUMIDITY", "UMIDADE") \
  X(Pressure, "PRESSURE", "PRESSÃO") \
  X(Wind, "WIND", "VENTO") \
  X(Forecast3h, "Forecast 3h", "Previsão 3h") \
  X(ForecastTomorrow, "Forecast tomorrow", "Previsão amanhã") \
  X(NoForecast, "No forecast", "Sem previsão") \
  X(ForecastNoData, "Forecast: no data from API", "Previsão: sem dados da API") \
  X(WeatherForLocation, "Weather for location: ", "Clima para localização: ") \
  /* matrix / game */ \
  X(Matrix, "MATRIX CLOCK", "RELÓGIO MATRIX") \
  X(GameTitle, "UNLIS HUNTER GAME", "JOGO UNLIS HUNTER") \
  X(GameHint1, "Use a Yagi antenna to catch", "Use uma antena Yagi para captar") \
  X(GameHint2, "and jam its signal by pressing", "e interferir no sinal pressionando") \
  X(GameHint3, "PTT on it.", "PTT nela.") \
  X(GameStart, "START", "INICIAR") \
  X(GameStop, "STOP", "PARAR") \
  X(GamePtt, "PTT", "PTT") \
  X(GameExit, "EXIT", "SAIR") \
  X(GameBand, "BAND:", "BANDA:") \
  X(GameOver, "GAME OVER", "FIM DE JOGO") \
  /* location */ \
  X(Location, "LOCATION", "LOCALIZAÇÃO") \
  X(GpsLabel, "GPS:", "GPS:") \
  X(GpsActive, "ACTIVE", "ATIVO") \
  X(GpsInactive, "INACTIVE", "INATIVO") \
  X(Fix, "FIX:", "FIX:") \
  X(Sats, "SATS:", "SATS:") \
  X(Lat, "LAT:", "LAT:") \
  X(Lon, "LON:", "LON:") \
  X(Speed, "SPEED:", "VELOC:") \
  X(Locator, "LOCATOR:", "LOCATOR:") \
  /* settings */ \
  X(Settings, "SETTINGS", "CONFIGURAÇÕES") \
  X(DisplaySettings, "DISPLAY SETTINGS", "AJUSTES DE TELA") \
  X(Brightness, "BRIGHTNESS", "BRILHO") \
  X(ThemeColor, "THEME COLOR", "COR DO TEMA") \
  X(Language, "LANGUAGE", "IDIOMA") \
  X(Dark, "DARK", "ESCURO") \
  X(Light, "LIGHT", "CLARO") \
  X(HoldHint, "Hold 3 s on any screen for this menu", "Segure 3 s em qualquer tela para este menu") \
  X(Calibrate, "CALIBRATE", "CALIBRAR") \
  X(CalibrateHint, "Touch the corners to calibrate", "Toque nos cantos para calibrar") \
  X(Rot90Right, "Rotate 90 right", "Girar 90 à direita") \
  X(Rot90Left, "Rotate 90 left", "Girar 90 à esquerda") \
  /* overlays */ \
  X(AprsAlertTitle, "APRS ALERT", "ALERTA APRS") \
  X(AprsBeaconing, "APRS BEACONING...", "APRS BEACON...") \
  X(WeatherAlert, "WEATHER ALERT", "ALERTA CLIMÁTICO") \
  X(PossibleStorm, "POSSIBLE STORM", "POSSÍVEL TEMPESTADE") \
  X(StormNow, "STORM NOW", "TEMPESTADE AGORA") \
  X(StrongWind, "STRONG WIND", "VENTO FORTE") \
  X(Repeat, "Repeat:", "Repetir:") \
  X(Path, "PATH:", "CAMINHO:") \
  X(Frame, "Frame:", "Quadro:") \
  X(Mhz, "MHz", "MHz") \
  X(Km, "km", "km") \
  X(KmH, "km/h", "km/h") \
  X(Bearing, "Bearing", "Direção") \
  X(WxTemp, "TEMP:", "TEMP:") \
  X(WxWind, "WIND:", "VENTO:") \
  /* boot */ \
  X(Booting, "System starting...", "Iniciando o sistema...") \
  X(Ready, "System ready!", "Sistema pronto!") \
  X(FollowPropagation, "FOLLOW THE PROPAGATION...", "ACOMPANHE A PROPAGAÇÃO...")

enum class Tr : uint16_t {
#define X(k, en, pt) k,
  TR_LIST(X)
#undef X
  Count
};

const char* tr(Tr key);
void i18nSetLang(Lang lang);
Lang langFromCode(const char* code);
