#include "zeromesh_notify.h"
#include <notification/notification.h>
#include <notification/notification_messages.h>

const char* ringtone_names[] = {
    "Off",
    "Short",
    "Double",
    "Triple",
    "Long",
    "SOS",
    "Chirp",
};

static const NotificationSequence seq_vibro_short = {
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    NULL,
};

static const NotificationSequence seq_led_flash = {
    &message_blue_255,
    &message_delay_250,
    &message_blue_0,
    &message_delay_100,
    &message_blue_255,
    &message_delay_250,
    &message_blue_0,
    NULL,
};

static const NotificationSequence seq_vibro_led = {
    &message_vibro_on,
    &message_blue_255,
    &message_delay_100,
    &message_vibro_off,
    &message_delay_50,
    &message_blue_0,
    &message_delay_100,
    &message_blue_255,
    &message_delay_250,
    &message_blue_0,
    NULL,
};

void play_ringtone(ZeroMeshApp* app) {
    if(app->notify_ringtone == RingtoneNone) return;

    if(!furi_hal_speaker_acquire(1000)) return;

    switch(app->notify_ringtone) {
    case RingtoneShort:
        furi_hal_speaker_start(800.0f, 0.6f);
        furi_delay_ms(100);
        furi_hal_speaker_stop();
        break;

    case RingtoneDouble:
        furi_hal_speaker_start(800.0f, 0.6f);
        furi_delay_ms(80);
        furi_hal_speaker_stop();
        furi_delay_ms(60);
        furi_hal_speaker_start(1000.0f, 0.6f);
        furi_delay_ms(80);
        furi_hal_speaker_stop();
        break;

    case RingtoneTriple:
        for(int i = 0; i < 3; i++) {
            furi_hal_speaker_start(800.0f + (float)(i * 200), 0.6f);
            furi_delay_ms(60);
            furi_hal_speaker_stop();
            if(i < 2) furi_delay_ms(40);
        }
        break;

    case RingtoneLong:
        furi_hal_speaker_start(600.0f, 0.5f);
        furi_delay_ms(400);
        furi_hal_speaker_stop();
        break;

    case RingtoneSOS:
        for(int i = 0; i < 3; i++) {
            furi_hal_speaker_start(800.0f, 0.6f);
            furi_delay_ms(50);
            furi_hal_speaker_stop();
            furi_delay_ms(50);
        }
        furi_delay_ms(100);
        for(int i = 0; i < 3; i++) {
            furi_hal_speaker_start(800.0f, 0.6f);
            furi_delay_ms(150);
            furi_hal_speaker_stop();
            furi_delay_ms(50);
        }
        furi_delay_ms(100);
        for(int i = 0; i < 3; i++) {
            furi_hal_speaker_start(800.0f, 0.6f);
            furi_delay_ms(50);
            furi_hal_speaker_stop();
            if(i < 2) furi_delay_ms(50);
        }
        break;

    case RingtoneChirp:
        for(int f = 400; f <= 1200; f += 50) {
            furi_hal_speaker_start((float)f, 0.5f);
            furi_delay_ms(15);
        }
        furi_hal_speaker_stop();
        break;

    default:
        break;
    }

    furi_hal_speaker_release();
}

void notify_rx_message(ZeroMeshApp* app) {
    NotificationApp* notify = furi_record_open(RECORD_NOTIFICATION);

    if(app->notify_vibro && app->notify_led) {
        notification_message(notify, &seq_vibro_led);
    } else if(app->notify_vibro) {
        notification_message(notify, &seq_vibro_short);
    } else if(app->notify_led) {
        notification_message(notify, &seq_led_flash);
    }

    furi_record_close(RECORD_NOTIFICATION);

    if(app->notify_ringtone != RingtoneNone) {
        play_ringtone(app);
    }
}