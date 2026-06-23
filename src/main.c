#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#define DS3231_I2C_ADDR 0x68U

#define BCD2BIN(val) ((((val) >> 4) * 10) + ((val) & 0x0F))
#define BIN2BCD(val) ((((val) / 10) << 4) + ((val) % 10))

struct ds3231_time {
    uint8_t sec, min, hour, wday, mday, month, year;
};

static const struct device *i2c_dev;

static int ds3231_read_time(struct ds3231_time *t)
{
    uint8_t reg = 0x00;
    uint8_t raw[7];
    int ret = i2c_write_read(i2c_dev, DS3231_I2C_ADDR, &reg, 1, raw, sizeof(raw));
    if (ret) {
        return ret;
    }
    t->sec   = BCD2BIN(raw[0] & 0x7F);
    t->min   = BCD2BIN(raw[1] & 0x7F);
    t->hour  = BCD2BIN(raw[2] & 0x3F);  
    t->wday  = raw[3] & 0x07;
    t->mday  = BCD2BIN(raw[4] & 0x3F);
    t->month = BCD2BIN(raw[5] & 0x1F);
    t->year  = BCD2BIN(raw[6]);
    return 0;
}

static int ds3231_set_time(const struct ds3231_time *t)
{
    uint8_t buf[8] = {
        0x00,
        BIN2BCD(t->sec), BIN2BCD(t->min), BIN2BCD(t->hour),
        t->wday, BIN2BCD(t->mday), BIN2BCD(t->month), BIN2BCD(t->year)
    };
    return i2c_write(i2c_dev, buf, sizeof(buf), DS3231_I2C_ADDR);
}

int main(void)
{
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        printk("I2C nao esta pronto\n");
        return -1;
    }

    /* roda uma vez pra acertar o relogio, depois comenta essas  linhas */
    struct ds3231_time set = { .sec=0, .min=15, .hour=22,
                                .wday=2, .mday=22, .month=6, .year=26 };
    ds3231_set_time(&set);

    while (1) {
        struct ds3231_time now;
        if (ds3231_read_time(&now) == 0) {
            printk("%02u:%02u:%02u  %02u/%02u/20%02u\n",
                   now.hour, now.min, now.sec, now.mday, now.month, now.year);
        }
        k_sleep(K_SECONDS(1));
    }
}