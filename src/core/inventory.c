#include "wc.h"
#include <string.h>

int wc_inventory_used(const WcInventory *inv) {
    int n = 0;
    for (int i = 0; i < WC_ITEMS; i++)
        n += inv->n[i];
    return n;
}
bool wc_add(WcInventory *inv, int item, int quantity) {
    if (item < 0 || item >= ITEM_COUNT || quantity < 0 || quantity > 10000)
        return false;
    if (wc_inventory_used(inv) > inv->capacity - quantity)
        return false;
    inv->n[item] += quantity;
    return true;
}
int wc_available(const WcState *s, int item) {
    if (item < 0 || item >= ITEM_COUNT)
        return 0;
    int n = s->backpack.n[item] + s->pantry.n[item];
    for (int i = 0; i < s->room_count; i++)
        n += s->rooms[i].cabinet.n[item];
    return n;
}
bool wc_pay(WcState *s, const int cost[WC_ITEMS], int home) {
    for (int i = 0; i < WC_ITEMS; i++)
        if (cost[i] < 0 || (home ? wc_available(s, i) : s->backpack.n[i]) < cost[i])
            return false;
    for (int i = 0; i < WC_ITEMS; i++) {
        int left = cost[i], n = s->backpack.n[i] < left ? s->backpack.n[i] : left;
        s->backpack.n[i] -= n;
        left -= n;
        /* Eligible resources are consumed first. Home stock is never at risk. */
        int eligible = s->eligible[i] < n ? s->eligible[i] : n;
        s->eligible[i] -= eligible;
        if (home) {
            n = s->pantry.n[i] < left ? s->pantry.n[i] : left;
            s->pantry.n[i] -= n;
            left -= n;
            for (int r = 0; r < s->room_count; r++) {
                n = s->rooms[r].cabinet.n[i] < left ? s->rooms[r].cabinet.n[i] : left;
                s->rooms[r].cabinet.n[i] -= n;
                left -= n;
            }
        }
    }
    return true;
}
void wc_xp(WcState *s, int amount, uint32_t achievement) {
    if (achievement && (s->achievements & achievement))
        return;
    s->achievements |= achievement;
    s->xp += amount;
    int earned = s->xp / 12;
    if (earned > 3)
        earned = 3;
    s->points += earned - s->awarded;
    s->awarded = earned;
}
