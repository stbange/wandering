#include "app.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, flush_storage, (), {
    FS.syncfs(
        false, function(err) {
            if (err) {
                Module.ccall('wc_web_action', null, [ 'number', 'number', 'number', 'number' ],
                             [ -99, 0, 0, 0 ]);
                console.error('Save storage failed', err);
            }
        });
});
#endif
static bool read_save(const char *path, WcState *out) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    unsigned char *bytes = malloc(WC_SAVE_MAX + 1);
    if (!bytes) {
        fclose(f);
        return false;
    }
    size_t n = fread(bytes, 1, WC_SAVE_MAX + 1, f);
    int error = ferror(f);
    fclose(f);
    bool ok = !error && wc_decode(out, bytes, n);
    free(bytes);
    return ok;
}
bool wc_platform_save(WcApp *a) {
    unsigned char *bytes = malloc(WC_SAVE_MAX);
    if (!bytes)
        return false;
    size_t n = wc_encode(&a->game.s, bytes, WC_SAVE_MAX);
    char tmp[600], prev[600];
    snprintf(tmp, sizeof(tmp), "%s.tmp", a->save_path);
    snprintf(prev, sizeof(prev), "%s.prev", a->save_path);
    FILE *f = n ? fopen(tmp, "wb") : NULL;
    int ok = 0;
    if (f) {
        ok = fwrite(bytes, 1, n, f) == n;
        if (fflush(f))
            ok = 0;
#ifndef __EMSCRIPTEN__
#ifdef _WIN32
        if (_commit(_fileno(f)))
            ok = 0;
#else
        if (fsync(fileno(f)))
            ok = 0;
#endif
#endif
        if (fclose(f))
            ok = 0;
    }
    free(bytes);
    if (ok) {
        WcState check;
        ok = read_save(tmp, &check);
    }
    if (ok) {
        /* Keep a verified prior generation; never replace it with corrupt bytes. */
        WcState check;
        if (read_save(a->save_path, &check)) {
            remove(prev);
            if (rename(a->save_path, prev))
                ok = 0;
        } else
            remove(a->save_path);
        if (ok && rename(tmp, a->save_path))
            ok = 0;
    }
    a->game.s.storage_failed = !ok;
    snprintf(a->game.message, sizeof(a->game.message), "%s",
             ok ? "Journey saved. Previous generation retained."
                : "Save failed. Exploration halted; your live progress is retained.");
#ifdef __EMSCRIPTEN__
    if (ok)
        flush_storage();
#endif
    return ok;
}
bool wc_platform_load(WcApp *a) {
    WcState next;
    char prev[600];
    snprintf(prev, sizeof(prev), "%s.prev", a->save_path);
    bool fallback = false, ok = read_save(a->save_path, &next);
    if (!ok) {
        ok = read_save(prev, &next);
        fallback = ok;
    }
    if (!ok) {
        snprintf(a->game.message, sizeof(a->game.message),
                 "No valid compatible save. Live journey retained.");
        return false;
    }
    a->game.s = next;
    wc_input_reset(a);
    wc_panel(a, PANEL_NONE);
    a->accumulator = 0;
    snprintf(a->game.message, sizeof(a->game.message), "%s",
             fallback ? "Previous save recovered. Parked and ready."
                      : "Journey restored. Parked and ready.");
    return true;
}
void wc_platform_sound(WcApp *a) {
    if (a->audio && a->sound) {
        SDL_ClearAudioStream(a->audio);
        SDL_PutAudioStreamData(a->audio, a->sound, (int)a->sound_length);
    }
}
