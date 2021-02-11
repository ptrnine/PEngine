#pragma once

#include "core/types.hpp"
#include "core/async.hpp"
#include "core/log.hpp"

namespace inp
{

enum class key : core::u32 {
    ESC              = 1,
    N1               = 2,
    N2               = 3,
    N3               = 4,
    N4               = 5,
    N5               = 6,
    N6               = 7,
    N7               = 8,
    N8               = 9,
    N9               = 10,
    N0               = 11,
    MINUS            = 12,
    EQUAL            = 13,
    BACKSPACE        = 14,
    TAB              = 15,
    Q                = 16,
    W                = 17,
    E                = 18,
    R                = 19,
    T                = 20,
    Y                = 21,
    U                = 22,
    I                = 23,
    O                = 24,
    P                = 25,
    LEFTBRACE        = 26,
    RIGHTBRACE       = 27,
    ENTER            = 28,
    LEFTCTRL         = 29,
    A                = 30,
    S                = 31,
    D                = 32,
    F                = 33,
    G                = 34,
    H                = 35,
    J                = 36,
    K                = 37,
    L                = 38,
    SEMICOLON        = 39,
    APOSTROPHE       = 40,
    GRAVE            = 41,
    LEFTSHIFT        = 42,
    BACKSLASH        = 43,
    Z                = 44,
    X                = 45,
    C                = 46,
    V                = 47,
    B                = 48,
    N                = 49,
    M                = 50,
    COMMA            = 51,
    DOT              = 52,
    SLASH            = 53,
    RIGHTSHIFT       = 54,
    KPASTERISK       = 55,
    LEFTALT          = 56,
    SPACE            = 57,
    CAPSLOCK         = 58,
    F1               = 59,
    F2               = 60,
    F3               = 61,
    F4               = 62,
    F5               = 63,
    F6               = 64,
    F7               = 65,
    F8               = 66,
    F9               = 67,
    F10              = 68,
    NUMLOCK          = 69,
    SCROLLLOCK       = 70,
    KP7              = 71,
    KP8              = 72,
    KP9              = 73,
    KPMINUS          = 74,
    KP4              = 75,
    KP5              = 76,
    KP6              = 77,
    KPPLUS           = 78,
    KP1              = 79,
    KP2              = 80,
    KP3              = 81,
    KP0              = 82,
    KPDOT            = 83,
    ZENKAKUHANKAKU   = 85,
    KEY_102ND        = 86,
    F11              = 87,
    F12              = 88,
    RO               = 89,
    KATAKANA         = 90,
    HIRAGANA         = 91,
    HENKAN           = 92,
    KATAKANAHIRAGANA = 93,
    MUHENKAN         = 94,
    KPJPCOMMA        = 95,
    KPENTER          = 96,
    RIGHTCTRL        = 97,
    KPSLASH          = 98,
    SYSRQ            = 99,
    RIGHTALT         = 100,
    LINEFEED         = 101,
    HOME             = 102,
    UP               = 103,
    PAGEUP           = 104,
    LEFT             = 105,
    RIGHT            = 106,
    END              = 107,
    DOWN             = 108,
    PAGEDOWN         = 109,
    INSERT           = 110,
    DELETE           = 111,
    MACRO            = 112,
    MUTE             = 113,
    VOLUMEDOWN       = 114,
    VOLUMEUP         = 115,
    POWER            = 116,
    KPEQUAL          = 117,
    KPPLUSMINUS      = 118,
    PAUSE            = 119,
    SCALE            = 120,
    KPCOMMA          = 121,
    HANGEUL          = 122,
    HANJA            = 123,
    YEN              = 124,
    LEFTMETA         = 125,
    RIGHTMETA        = 126,
    COMPOSE          = 127,
    STOP             = 128,
    AGAIN            = 129,
    PROPS            = 130,
    UNDO             = 131,
    FRONT            = 132,
    COPY             = 133,
    OPEN             = 134,
    PASTE            = 135,
    FIND             = 136,
    CUT              = 137,
    HELP             = 138,
    MENU             = 139,
    CALC             = 140,
    SETUP            = 141,
    SLEEP            = 142,
    WAKEUP           = 143,
    FILE             = 144,
    SENDFILE         = 145,
    DELETEFILE       = 146,
    XFER             = 147,
    PROG1            = 148,
    PROG2            = 149,
    WWW              = 150,
    MSDOS            = 151,
    COFFEE           = 152,
    SCREENLOCK       = COFFEE,
    ROTATE_DISPLAY   = 153,
    DIRECTION        = ROTATE_DISPLAY,
    CYCLEWINDOWS     = 154,
    MAIL             = 155,
    BOOKMARKS        = 156,
    COMPUTER         = 157,
    BACK             = 158,
    FORWARD          = 159,
    CLOSECD          = 160,
    EJECTCD          = 161,
    EJECTCLOSECD     = 162,
    NEXTSONG         = 163,
    PLAYPAUSE        = 164,
    PREVIOUSSONG     = 165,
    STOPCD           = 166,
    RECORD           = 167,
    REWIND           = 168,
    PHONE            = 169,
    ISO              = 170,
    CONFIG           = 171,
    HOMEPAGE         = 172,
    REFRESH          = 173,
    EXIT             = 174,
    MOVE             = 175,
    EDIT             = 176,
    SCROLLUP         = 177,
    SCROLLDOWN       = 178,
    KPLEFTPAREN      = 179,
    KPRIGHTPAREN     = 180,
    NEW              = 181,
    REDO             = 182,
    F13              = 183,
    F14              = 184,
    F15              = 185,
    F16              = 186,
    F17              = 187,
    F18              = 188,
    F19              = 189,
    F20              = 190,
    F21              = 191,
    F22              = 192,
    F23              = 193,
    F24              = 194,
    PLAYCD           = 200,
    PAUSECD          = 201,
    PROG3            = 202,
    PROG4            = 203,
    DASHBOARD        = 204,
    SUSPEND          = 205,
    CLOSE            = 206,
    PLAY             = 207,
    FASTFORWARD      = 208,
    BASSBOOST        = 209,
    PRINT            = 210,
    HP               = 211,
    CAMERA           = 212,
    SOUND            = 213,
    QUESTION         = 214,
    EMAIL            = 215,
    CHAT             = 216,
    SEARCH           = 217,
    CONNECT          = 218,
    FINANCE          = 219,
    SPORT            = 220,
    SHOP             = 221,
    ALTERASE         = 222,
    CANCEL           = 223,
    BRIGHTNESSDOWN   = 224,
    BRIGHTNESSUP     = 225,
    MEDIA            = 226,
    SWITCHVIDEOMODE  = 227,
    KBDILLUMTOGGLE   = 228,
    KBDILLUMDOWN     = 229,
    KBDILLUMUP       = 230,
    SEND             = 231,
    REPLY            = 232,
    FORWARDMAIL      = 233,
    SAVE             = 234,
    DOCUMENTS        = 235,
    BATTERY          = 236,
    BLUETOOTH        = 237,
    WLAN             = 238,
    UWB              = 239,
    UNKNOWN          = 240,
    VIDEO_NEXT       = 241,
    VIDEO_PREV       = 242,
    BRIGHTNESS_CYCLE = 243,
    BRIGHTNESS_AUTO  = 244,
    DISPLAY_OFF      = 245,
    WWAN             = 246,
    RFKILL           = 247,
    MICMUTE          = 248,
    BTN_MISC         = 0x100,
    BTN_0            = 0x100,
    BTN_1            = 0x101,
    BTN_2            = 0x102,
    BTN_3            = 0x103,
    BTN_4            = 0x104,
    BTN_5            = 0x105,
    BTN_6            = 0x106,
    BTN_7            = 0x107,
    BTN_8            = 0x108,
    BTN_9            = 0x109,
    BTN_MOUSE        = 0x110,
    BTN_LEFT         = 0x110,
    BTN_RIGHT        = 0x111,
    BTN_MIDDLE       = 0x112,
    BTN_SIDE         = 0x113,
    BTN_EXTRA        = 0x114,
    BTN_FORWARD      = 0x115,
    BTN_BACK         = 0x116,
    BTN_TASK         = 0x117
};

enum class track : core::u32 { mouse_x = 0, mouse_y = 1, mouse_scroll = 8 };

template <size_t KeyShift>
struct keycode_holder {
    keycode_holder() {
        k.store(0);
    }

    void set_press(core::u32 keycode) {
        k.fetch_or(1LLU << ((keycode - key_shift) * 2));
    }

    void set_release(core::u32 keycode) {
        k.fetch_or(1LLU << (((keycode - key_shift) * 2) + 1));
    }

    template <size_t keycode>
    bool pop_press() {
        constexpr auto mask  = 1LLU << ((keycode - key_shift) * 2);
        constexpr auto imask = ~mask;
        core::u64 v = 0;
        v = k.exchange(v);

        bool test = v & mask;
        v &= imask;

        k.fetch_or(v);
        return test;
    }

    template <size_t keycode>
    bool pop_release() {
        constexpr auto mask  = 1LLU << ((keycode - key_shift) * 2 + 1);
        constexpr auto imask = ~mask;
        core::u64 v = 0;
        v = k.exchange(v);

        bool test = v & mask;
        v &= imask;

        k.fetch_or(v);
        return test;
    }

    static constexpr size_t key_shift = KeyShift;
    std::atomic<core::u64> k;
};

template <typename T>
concept KeycodeHolder = requires (T v) {
    {v.k.load()}  -> std::same_as<core::u64>;
    {v.key_shift} -> std::same_as<size_t>;
};

constexpr size_t EPQ        = 32; // Events per qword
constexpr size_t MAX_QWORDS = 9;  // qwords count

namespace details {
    struct helper_f {
        constexpr auto operator()(auto v) {
            return core::hana::type_c<keycode_holder<v * EPQ>>;
        }
    };

    using keycode_storage = decltype(core::hana::unpack(
        core::hana::transform(core::hana::to_tuple(core::hana::range_c<size_t, 0, MAX_QWORDS>),
                              helper_f{}),
        core::hana::template_<core::hana::tuple>))::type;
} // namespace details

using keycode_storage   = details::keycode_storage;
using keycodes_snapshot = core::array<core::optional<core::u64>, MAX_QWORDS>;
using keycodes_snapshot_updated = core::array<core::u64, MAX_QWORDS>;

using trackpos = core::i16;
struct trackpos_holder {
    std::atomic<trackpos> t;
};

constexpr size_t TRACKS_COUNT = 16;
using trackpos_storage        = core::array<trackpos_holder, TRACKS_COUNT>;
using trackpos_snapshot       = core::array<core::optional<trackpos>, TRACKS_COUNT>;


class inp_event_transaction;

class inp_event_receiver : public std::enable_shared_from_this<inp_event_receiver> {
public:
    friend class inp_event_transaction;
    friend class inp_linux;

    static core::shared_ptr<inp_event_receiver> create_shared() {
        return core::make_shared<inp_event_receiver>(
            core::constructor_accessor<inp_event_receiver>{});
    }

    inp_event_receiver(core::constructor_accessor<inp_event_receiver>::cref) {}

    template <core::u32 keycode>
    bool pop_pressed() {
        constexpr size_t kc = keycode / EPQ;
        return core::hana::at_c<kc>(keys).template pop_press<keycode>();
    }

    template <key keycode>
    bool pop_pressed() {
        return pop_pressed<static_cast<core::u32>(keycode)>();
    }

    template <core::u32 keycode>
    bool pop_released() {
        constexpr size_t kc = keycode / EPQ;
        return core::hana::at_c<kc>(keys).template pop_release<keycode>();
    }

    template <key keycode>
    bool pop_released() {
        return pop_released<static_cast<core::u32>(keycode)>();
    }

    template <size_t TrackNum>
    core::i16 pop_trackpos() {
        core::i16 v = 0;
        v = get<TrackNum>(tracks).t.exchange(v);
        return v;
    }

    core::i16 pop_trackpos(size_t track_num) {
        core::i16 v = 0;
        v = tracks[track_num].t.exchange(v);
        return v;
    }

    template <track TrackNum>
    core::i16 pop_trackpos() {
        return pop_trackpos<static_cast<core::u32>(TrackNum)>();
    }

    core::i16 pop_trackpos(track track_num) {
        return pop_trackpos(static_cast<core::u32>(track_num));
    }

    inp_event_transaction start_transaction();

protected:
    void update_track(size_t track_num, trackpos pos) {
        Expects(track_num < TRACKS_COUNT);
        tracks[track_num].t.fetch_add(pos);
    }

    void set_press(core::u32 keycode) {
        auto kc = keycode / EPQ;
        core::hana::for_each(keys, [kc, keycode](auto& v) {
            if (kc == v.key_shift / EPQ)
                v.set_press(keycode);
        });
    }

    void set_release(core::u32 keycode) {
        auto kc = keycode / EPQ;
        core::hana::for_each(keys, [kc, keycode](auto& v) {
            if (kc == v.key_shift / EPQ)
                v.set_release(keycode);
        });
    }

private:
    keycode_storage  keys;
    trackpos_storage tracks;
    bool             on_transaction = false;
};

class inp_event_transaction {
public:
    inp_event_transaction(core::shared_ptr<inp_event_receiver> ireceiver): // NOLINT
        receiver(core::move(ireceiver)) {}

    inp_event_transaction(inp_event_transaction&&) = default;
    inp_event_transaction& operator=(inp_event_transaction&&) = default;

    inp_event_transaction(const inp_event_transaction&) = delete;
    inp_event_transaction& operator=(const inp_event_transaction&) = delete;

    template <core::u32 keycode, size_t BitN>
    bool test() {
        namespace h = core::hana;

        constexpr size_t kc = keycode / EPQ;
        auto& keyopt        = get<kc>(keys);
        auto& keynew        = get<kc>(keys_new);

        if (!keyopt) {
            auto& receiver_keys = receiver->keys;
            keynew = 0;
            get<kc>(keys_new) = h::at_c<kc>(receiver_keys).k.exchange(get<kc>(keys_new));
            keyopt = keynew;
        }

        constexpr auto key_shift = kc * EPQ;
        constexpr auto mask  = 1LLU << ((keycode - key_shift) * 2 + BitN);
        constexpr auto imask = ~mask;

        keynew &= imask;

        return *keyopt & mask;
    }

    template <size_t BitN>
    bool test(core::u32 keycode) {
        namespace h = core::hana;

        size_t kc = keycode / EPQ;
        auto& keyopt = keys[kc];
        auto& keynew = keys_new[kc];

        if (!keyopt) {
            auto& receiver_keys = receiver->keys;
            keynew = 0;
            auto& key_new = keys_new[kc];
            h::for_each(receiver_keys, [kc, &key_new](auto& v) {
                if (kc == v.key_shift / EPQ)
                    key_new = v.k.exchange(key_new);
            });
            keyopt = keynew;
        }

        auto key_shift = kc * EPQ;
        auto mask  = 1LLU << ((keycode - key_shift) * 2 + BitN);
        auto imask = ~mask;

        keynew &= imask;

        return *keyopt & mask;
    }

    template <core::u32 keycode>
    bool is_pressed() {
        return test<keycode, 0>();
    }

    template <core::u32 keycode>
    bool is_released() {
        return test<keycode, 1>();
    }

    template <key keycode>
    bool is_pressed() {
        return is_pressed<static_cast<core::u32>(keycode)>();
    }

    template <key keycode>
    bool is_released() {
        return is_released<static_cast<core::u32>(keycode)>();
    }

    bool is_pressed(core::u32 keycode) {
        return test<0>(keycode);
    }

    bool is_released(core::u32 keycode) {
        return test<1>(keycode);
    }

    bool is_pressed(key keycode) {
        return is_pressed(static_cast<core::u32>(keycode));
    }

    bool is_released(key keycode) {
        return is_released(static_cast<core::u32>(keycode));
    }

    template <size_t TrackNum>
    core::i16 trackpos() {
        auto& trackopt = get<TrackNum>(tracks);

        if (!trackopt) {
            auto& receiver_tracks = receiver->tracks;
            trackopt = 0;
            *trackopt = get<TrackNum>(receiver_tracks).t.exchange(*trackopt);
        }

        return *trackopt;
    }

    template <track TrackNum>
    core::i16 trackpos() {
        return trackpos<static_cast<core::u32>(TrackNum)>();
    }

    core::i16 trackpos(size_t track_num) {
        auto& trackopt = tracks[track_num];

        if (!trackopt) {
            auto& receiver_tracks = receiver->tracks;
            trackopt = 0;
            *trackopt = receiver_tracks[track_num].t.exchange(*trackopt); // NOLINT
        }

        return *trackopt;
    }

    core::i16 trackpos(track track_num) {
        return trackpos(static_cast<core::u32>(track_num));
    }

    template <size_t TrackNum>
    float trackpos_float() {
        return static_cast<float>(trackpos<TrackNum>());
    }

    template <track TrackNum>
    float trackpos_float() {
        return trackpos_float<static_cast<core::u32>(TrackNum)>();
    }

    float trackpos_float(size_t track_num) {
        return static_cast<float>(trackpos(track_num));
    }

    float trackpos_float(track track_num) {
        return trackpos_float(static_cast<core::u32>(track_num));
    }

    ~inp_event_transaction() {
        namespace h = core::hana;
        using namespace h::literals;

        if (!receiver)
            return;

        auto ks = h::transform(receiver->keys, [](auto& v) { return &v; });
        h::for_each(h::zip(ks, h::to_tuple(h::range_c<size_t, 0, MAX_QWORDS>)), [&](const auto& p) {
            if (keys[p[1_c]])
                p[0_c]->k.fetch_or(keys_new[p[1_c]]);
        });

        receiver->on_transaction = false;
    }

private:
    core::shared_ptr<inp_event_receiver> receiver;
    keycodes_snapshot                    keys;
    keycodes_snapshot_updated            keys_new; // NOLINT
    trackpos_snapshot                    tracks;
};


class inp_event_states {
public:
    inp_event_states() = default;

    template <size_t S>
    inp_event_states(const core::array<key, S>& updated_keys) {
        for (auto k : updated_keys)
            keys.emplace(k, false);
    }

    template <size_t S>
    void setup_states(const core::array<key, S>& updated_keys) {
        keys.clear();
        for (auto k : updated_keys)
            keys.emplace(k, false);
    }

    void update(inp_event_transaction& t) {
        for (auto& [k, s] : keys) {
            if (t.is_pressed(k))
                s = true;
            if (t.is_released(k))
                s = false;
        }
    }

    [[nodiscard]]
    bool state_of(key keycode) const {
        auto found = keys.find(keycode);
        if (found == keys.end()) {
            LOG_WARNING("Keycode [{}] does not exist in inp_event_states!");
            return false;
        }
        return found->second;
    }

private:
    core::hash_map<key, bool> keys;
};


inline inp_event_transaction inp_event_receiver::start_transaction() {
    if (on_transaction)
        throw std::runtime_error("Attempt to start more than one transaction at the same time");

    on_transaction = true;

    return inp_event_transaction(shared_from_this());
}

} // namespace inp
