#include <lvgl.h>
#include <LilyGoLib.h>
#include <Preferences.h>  // ESP32 NVS Library

#include "config.h"
#include "settings.h"
#include "mbedtls/md.h"  // Standardmäßig im ESP32 vorhanden!
#include "dataCollection.h"
#include "locationData.h"
#include "weatherData.h"

// Preferences prefs;

extern uint8_t binKey[32];  // Puffer für das binäre Secret
extern int keyLen;

extern weatherData currentWeather;

void setupTOTP(String base32Secret) {
  base32Secret.replace(" ", "");  // Leerzeichen entfernen
  int len = base32Secret.length();
  if (len == 0) {
    Serial.println("setupTOTP Secret length 0");
    return;
  }

  keyLen = 0;

  uint32_t buffer = 0;
  int bitsLeft = 0;

  // Base32 zu Byte-Array Konvertierung
  for (int i = 0; i < len; i++) {
    char c = toupper(base32Secret[i]);
    int val = (c >= 'A' && c <= 'Z') ? c - 'A' : (c >= '2' && c <= '7') ? c - '2' + 26
                                                                        : -1;
    if (val == -1) continue;

    buffer = (buffer << 5) | val;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      binKey[keyLen++] = (buffer >> (bitsLeft - 8)) & 0xFF;
      bitsLeft -= 8;
    }
  }
}

String calculateTotpCode() {
  if (keyLen == 0) {
    Serial.println("calculateTotpCode No key");
    return "";
  }

  // 2. Zeit-Schritt berechnen
  time_t now;
  time(&now);
  now -= currentWeather.offset;  // Zeitzone + Sommerzeit
  Serial.println(now);

  uint64_t timesteps = now / 30;

  // 3. HMAC-SHA1 berechnen
  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) {
    msg[i] = timesteps & 0xFF;
    timesteps >>= 8;
  }

  uint8_t hash[20];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
  mbedtls_md_hmac_starts(&ctx, binKey, keyLen);
  mbedtls_md_hmac_update(&ctx, msg, 8);
  mbedtls_md_hmac_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  // 4. Truncation (Den 6-stelligen Code aus dem Hash extrahieren)
  int offset = hash[19] & 0x0F;
  uint32_t truncatedHash = (hash[offset] & 0x7F) << 24 | (hash[offset + 1] & 0xFF) << 16 | (hash[offset + 2] & 0xFF) << 8 | (hash[offset + 3] & 0xFF);

  uint32_t finalCode = truncatedHash % 1000000;

  // Als 6-stelligen String mit führenden Nullen zurückgeben
  char buf[8];
  sprintf(buf, "%06u", finalCode);
  return String(buf);
}
