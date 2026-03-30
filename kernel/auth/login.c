/* ============================================
 * Hardcoded login gate
 * ============================================ */

#include "login.h"
#include "../kernel.h"
#include "../io/line_input.h"
#include "../drivers/vga.h"

static const char LOGIN_USERNAME[] = "amarnath";
static const char LOGIN_PASSWORD[] = "pandey12";

static bool credentials_match(const char* input, size_t input_len, const char* expected, size_t expected_len)
{
    if (input_len != expected_len) {
        return false;
    }
    for (size_t i = 0; i < expected_len; i++) {
        if (input[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

void login_run(void)
{
    char user_buf[64];
    char pass_buf[64];
    const size_t user_len_expected = sizeof(LOGIN_USERNAME) - 1;
    const size_t pass_len_expected = sizeof(LOGIN_PASSWORD) - 1;

    for (;;) {
        vga_print("Username: ");
        size_t ulen = line_input_read(user_buf, sizeof(user_buf), true);
        vga_print("\nPassword: ");
        size_t plen = line_input_read(pass_buf, sizeof(pass_buf), false);
        vga_print("\n");

        if (credentials_match(user_buf, ulen, LOGIN_USERNAME, user_len_expected)
            && credentials_match(pass_buf, plen, LOGIN_PASSWORD, pass_len_expected)) {
            vga_print("Welcome, amarnath.\n");
            return;
        }

        vga_print("Login failed.\n\n");
    }
}
