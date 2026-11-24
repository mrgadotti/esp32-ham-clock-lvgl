#pragma once
#include <stdint.h>

enum class Lang : uint8_t { EN = 0, PT_BR = 1 };

// X(key, English, Brazilian Portuguese). Single source for the enum and both tables.
#define TR_LIST(X) \
  /* shared */ \
  X(Utc, "UTC", "UTC") \
  X(LocalTime, "Local Time", "Hora Local") \
  X(Ok, "OK", "OK") \
  X(Cancel, "CANCEL", "CANCELAR") \
  X(All, "ALL", "TODOS") \
  X(Mode, "MODE", "MODO") \
  X(Band, "BAND", "BANDA") \
  X(NoData, "No data", "Sem dados") \
  X(NoApiKey, "OpenWeather API key not set", "Chave da OpenWeather não configurada") \
  X(WaitingForData, "WAITING FOR DATA...", "AGUARDANDO DADOS...") \
  X(UpdatedUtc, "UPDATED UTC", "ATUALIZADO UTC") \
  X(UpdatedMinAgo, "updated %d min ago", "atualizado há %d min") \
  X(NoConnection, "NO CONNECTION", "SEM CONEXÃO") \
  /* clock */ \
  X(Clock, "HAM CLOCK", "RELÓGIO HAM") \
  X(OperatorStation, "OPERATOR STATION", "ESTAÇÃO DO OPERADOR") \
  /* table columns */ \
  X(ColCall, "CALL", "CALL") \
  X(ColMod, "MOD", "MOD") \
  X(ColMsg, "MSG", "MSG") \
  X(ColLocal, "LOC", "LOC") \
  X(ColCountry, "COUNTRY", "PAÍS") \
  X(ColSymbol, "SYM", "SYM") \
  X(ColKm, "KM", "KM") \
  X(ColFreq, "FREQ", "FREQ") \
  X(ColDay, "DAY", "DIA") \
  X(ColNight, "NIGHT", "NOITE") \
  /* DX cluster / POTA / HamAlert */ \
  X(DxCluster, "DX CLUSTER", "DX CLUSTER") \
  X(Pota, "POTA.app", "POTA.app") \
  X(HamAlert, "HAMALERT.org", "HAMALERT.org") \
  X(WaitingForSpots, "WAITING FOR SPOTS...", "AGUARDANDO SPOTS...") \
  X(FetchFailedRetrying, "FETCH FAILED, RETRYING...", "FALHA NA BUSCA, TENTANDO DE NOVO...") \
  X(HamAlertNoLogin, "HAMALERT LOGIN NOT SET", "LOGIN HAMALERT NÃO DEFINIDO") \
  X(Filters, "FILTERS", "FILTROS") \
  X(FilterCw, "CW", "CW") \
  X(FilterSsb, "SSB", "SSB") \
  X(FilterDigi, "DIGI", "DIGI") \
  /* APRS */ \
  X(AprsIs, "APRS-IS", "APRS-IS") \
  X(AprsKiss, "APRS-TNC-KISS", "APRS-TNC-KISS") \
  X(AprsTnc2, "APRS-TNC2-UART", "APRS-TNC2-UART") \
  X(AprsRadar, "APRS RADAR", "RADAR APRS") \
  X(AprsIsSta, "STATION", "ESTAÇÃO") \
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
  X(WeatherOm, "Weather - Open-Meteo", "Clima - Open-Meteo") \
  X(ForecastOm, "Forecast - Open-Meteo", "Previsão - Open-Meteo") \
  X(Temperature, "TEMPERATURE", "TEMPERATURA") \
  X(Humidity, "HUMIDITY", "UMIDADE") \
  X(Pressure, "PRESSURE", "PRESSÃO") \
  X(Wind, "WIND", "VENTO") \
  X(NoForecast, "No forecast", "Sem previsão") \
  X(ForecastNoData, "Forecast: no data from API", "Previsão: sem dados da API") \
  X(WeatherForLocation, "Weather for location: ", "Clima para localização: ") \
  /* settings */ \
  X(Settings, "SETTINGS", "CONFIGURAÇÕES") \
  X(Brightness, "BRIGHTNESS", "BRILHO") \
  X(Language, "LANGUAGE", "IDIOMA") \
  X(NightDim, "Night dimming", "Escurecer à noite") \
  X(CalibrateTouch, "Calibrate touch", "Calibrar toque") \
  X(InvertColors, "Invert colors", "Inverter cores") \
  X(Rotation, "Rotation", "Rotação") \
  X(AutoSwitch, "Auto-switch", "Troca automática") \
  X(AlertTime, "Alert time", "Tempo do alerta") \
  X(AprsRadius, "APRS radius", "Raio APRS") \
  X(BeaconNow, "Beacon now", "Beacon agora") \
  X(ClearAprsList, "Clear APRS list", "Limpar lista APRS") \
  X(ClearDxList, "Clear DX list", "Limpar lista DX") \
  X(Off, "Off", "Desligado") \
  X(EnabledScreens, "Enabled screens", "Telas ativas") \
  X(FreeHeap, "Free heap", "Heap livre") \
  X(Firmware, "Firmware", "Firmware") \
  /* overlays */ \
  X(AprsAlertTitle, "APRS ALERT", "ALERTA APRS") \
  X(AprsBeaconing, "APRS BEACONING...", "APRS BEACON...") \
  X(WeatherAlert, "WEATHER ALERT", "ALERTA CLIMÁTICO") \
  X(PossibleStorm, "POSSIBLE STORM", "POSSÍVEL TEMPESTADE") \
  X(StormNow, "STORM NOW", "TEMPESTADE AGORA") \
  X(StrongWind, "STRONG WIND", "VENTO FORTE") \
  X(Repeat, "Repeat:", "Repetir:") \
  X(Mhz, "MHz", "MHz") \
  X(Km, "km", "km") \
  X(KmH, "km/h", "km/h") \
  X(WxTemp, "TEMP:", "TEMP:") \
  X(WxWind, "WIND:", "VENTO:") \
  X(WxCondClear, "Clear sky", "Céu limpo") \
  X(WxCondPartly, "Partly cloudy", "Parcialmente nublado") \
  X(WxCondOvercast, "Overcast", "Nublado") \
  X(WxCondFog, "Fog", "Neblina") \
  X(WxCondDrizzle, "Drizzle", "Garoa") \
  X(WxCondRain, "Rain", "Chuva") \
  X(WxCondFreezingRain, "Freezing rain", "Chuva congelante") \
  X(WxCondShowers, "Showers", "Pancadas de chuva") \
  X(WxCondSnow, "Snow", "Neve") \
  X(WxCondThunder, "Thunderstorm", "Trovoada") \
  X(WxCondUnknown, "--", "--") \
  /* forecast, units, dates (day/month blocks must stay contiguous, Sunday/January first) */ \
  X(Plus3h, "+3h", "+3h") \
  X(Plus6h, "+6h", "+6h") \
  X(Ms, "m/s", "m/s") \
  X(HPa, "hPa", "hPa") \
  X(Min, "min", "min") \
  X(DaySun, "Sun", "Dom") \
  X(DayMon, "Mon", "Seg") \
  X(DayTue, "Tue", "Ter") \
  X(DayWed, "Wed", "Qua") \
  X(DayThu, "Thu", "Qui") \
  X(DayFri, "Fri", "Sex") \
  X(DaySat, "Sat", "Sáb") \
  X(WeekdaySunday, "Sunday", "Domingo") \
  X(WeekdayMonday, "Monday", "Segunda") \
  X(WeekdayTuesday, "Tuesday", "Terça") \
  X(WeekdayWednesday, "Wednesday", "Quarta") \
  X(WeekdayThursday, "Thursday", "Quinta") \
  X(WeekdayFriday, "Friday", "Sexta") \
  X(WeekdaySaturday, "Saturday", "Sábado") \
  X(MonthJanuary, "January", "janeiro") \
  X(MonthFebruary, "February", "fevereiro") \
  X(MonthMarch, "March", "março") \
  X(MonthApril, "April", "abril") \
  X(MonthMay, "May", "maio") \
  X(MonthJune, "June", "junho") \
  X(MonthJuly, "July", "julho") \
  X(MonthAugust, "August", "agosto") \
  X(MonthSeptember, "September", "setembro") \
  X(MonthOctober, "October", "outubro") \
  X(MonthNovember, "November", "novembro") \
  X(MonthDecember, "December", "dezembro") \
  /* alert overlay labels */ \
  X(Gust, "Gust:", "Rajada:") \
  X(WindLabel, "Wind:", "Vento:") \
  X(HumidityLabel, "HUMIDITY:", "UMIDADE:") \
  X(PressureLabel, "PRESSURE:", "PRESSÃO:") \
  /* APRS radar */ \
  X(RadarNorth, "N", "N") \
  X(RadarZoomReset, "1×", "1×") \
  /* APRS symbol short names (table order) */ \
  X(SymHouse, "HOUSE", "CASA") \
  X(SymCar, "CAR", "CARRO") \
  X(SymDigi, "DIGI", "DIGI") \
  X(SymRepeater, "REPEATER", "REPET.") \
  X(SymHuman, "HUMAN", "PESSOA") \
  X(SymLaptop, "LAPTOP", "LAPTOP") \
  X(SymLogin, "LOGIN", "LOGIN") \
  X(SymWx, "WX", "WX") \
  X(SymShip, "SHIP", "NAVIO") \
  X(SymAmbulance, "AMBUL", "AMBUL") \
  X(SymBike, "BIKE", "BIKE") \
  X(SymIcp, "ICP", "ICP") \
  X(SymFire, "FIRE", "BOMBEIRO") \
  X(SymFiretruck, "FIRETRK", "AUTOB.") \
  X(SymHospital, "HOSP", "HOSP") \
  X(SymYagi, "YAGI", "YAGI") \
  X(SymGlider, "GLIDER", "PLANADOR") \
  X(SymTruck, "TRUCK", "CAMINHÃO") \
  X(SymVan, "VAN", "VAN") \
  X(SymHamClock, "HAMCLOCK", "HAMCLOCK") \
  X(SymRover, "ROVER", "ROVER") \
  X(SymEoc, "EOC", "EOC") \
  X(SymTruckStop, "TRKSTOP", "POSTO") \
  X(SymMicERepeater, "MICERPT", "MICERPT") \
  X(SymNode, "NODE", "NÓ") \
  X(SymGrid, "GRID", "GRID") \
  X(SymLora, "LORA", "LORA") \
  /* APRS alert symbol descriptions */ \
  X(DescDigi, "Digipeater", "Digipeater") \
  X(DescRepeater, "Repeater", "Repetidora") \
  X(DescLaptop, "Laptop", "Laptop") \
  X(DescWx, "Weather station", "Estação meteo.") \
  X(DescShip, "Ship", "Navio") \
  X(DescAmbulance, "Ambulance", "Ambulância") \
  X(DescBike, "Bicycle", "Bicicleta") \
  X(DescFire, "Fire department", "Bombeiros") \
  X(DescHospital, "Hospital", "Hospital") \
  X(DescYagi, "Yagi antenna", "Antena Yagi") \
  X(DescGlider, "Glider", "Planador") \
  X(DescTruck, "Truck", "Caminhão") \
  X(DescVan, "Van", "Van") \
  X(DescRover, "Rover", "Rover") \
  X(DescNode, "Network node", "Nó de rede") \
  X(DescLora, "LoRa station", "Estação LoRa") \
  X(DescOther, "Station", "Estação") \
  /* touch calibration */ \
  X(CalTouch, "Touch the cross", "Toque na cruz") \
  X(CalVerify, "Verify: touch the cross", "Verificar: toque na cruz") \
  X(CalBad, "Bad points, retry", "Pontos ruins, tente de novo") \
  X(CalFailed, "Too inaccurate, retry", "Impreciso, tente de novo") \
  /* boot */ \
  X(Booting, "System starting...", "Iniciando o sistema...")

enum class Tr : uint16_t {
#define X(k, en, pt) k,
  TR_LIST(X)
#undef X
  Count
};

const char* tr(Tr key);
void i18nSetLang(Lang lang);
Lang langFromCode(const char* code);
