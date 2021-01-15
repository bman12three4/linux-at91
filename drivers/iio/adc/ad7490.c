/*
 * AD7490.c Analog Devices 12-bit 16-channel ADC driver
 * 
 * Byron Lathi
 */

#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define AD7490_MASK_CHANNEL_SEL		GENMASK(9, 6)
#define AD7490_CHANNEL_OFFSET		6
#define AD7490_MASK_TOTAL		GENMASK(11, 0)

struct ad7490_adc_chip {
	struct mutex lock;
	struct regulator *vref;
	struct iio_dev *indio_dev;
	struct spi_device *spi;
	u16 ctrl;
	unsigned int current_channel;
	u32 buffer ____cacheline_aligned;
};

static int ad7490_spi_write_ctrl(struct ad7490_adc_chip *ad7490_adc, u16 val,
				u16 mask)
{
	int ret;
	struct spi_message msg;
	struct spi_transfer tx[] = {
		{
			.tx_buf = &ad7490_adc->buffer,
			.len = 4,
		},
	};

	ad7490_adc->ctrl = (val & mask) | (ad7490_adc->ctrl & ~mask);
	ad7490_adc->buffer = ad7490_adc->ctrl;
	spi_message_init_with_transfers(&msg, tx, 1);
	ret = spi_sync(ad7490_adc->spi, &msg);


}

static int ad7490_spi_read_channel(struct ad7490_adc_chip *ad7490_adc, int* val,
				unsigned int channel)
{
	int ret;
	int mask = GENMASK(12, 0);
	struct spi_message msg;
	struct spi_transfer rx[] = {
		{
			.rx_buf = &ad7490_adc->buffer,
			.len = 4,
		},
	};

	ret = ret = ad7490_spi_write_ctrl(ad7490_adc,
					channel << AD7490_CHANNEL_OFFSET,
					AD7490_MASK_CHANNEL_SEL);

	if (ret)
		return ret;

	ad7490_adc->buffer = 0;
	spi_message_init_with_transfers(&msg, rx, 1);
	ret = spi_sync(ad7490_adc->spi, &msg);
	if (ret)
		return ret;

	ndelay(50);

	ad7490_adc->current_channel = channel;

	*val = ad7490_adc->buffer & mask;

	return 0;

}

#define AD7490_ADC_CHANNEL(chan) {				\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = (chan),					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
}

static int ad7490_spi_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct ad7490_adc_chip *ad7490_adc = iio_priv(indio_dev);
	int ret;

	if (!val)
		return -EINVAL;

	switch (mask) {
	case (IIO_CHAN_INFO_RAW):
		mutex_lock(&ad7490_adc->lock);
		ret = ad7490_spi_read_channel(ad7490_adc, val, chan->channel);
		mutex_unlock(&ad7490_adc->lock)

		if (ret < 0)
			return ret;

		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		ret = regulator_get_voltage(ad7490_adc->vref);
		if (ret < 0)
			return = ret;
		
		*val = ret / 5000;	//TODO: make this the right value
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static int ad7490_spi_reg_access(struct iio_dev *indio_dev,
				unsigned int reg, unsigned int writeval,
				unsigned int *readval)
{
	struct ad7490_adc_chip *ad7490_adc = iio_priv(indio_dev);
	int ret = 0;

	if (readval)
		*readval = ad7490_adc->ctrl;
	else
		ret = ad7490_spi_write_ctrl(ad7490_adc,
			writeval & AD7490_MASK_TOTAL, AD7490_MASK_TOTAL);
	
	return ret;
}

static const struct iio_info ad7490_spi_info = {
	.read_raw = ad7490_spi_read_raw,
	.debugfs_reg_access = ad7490_spi_reg_access,
};

int ad7490_spi_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct ad7490_adc_chip *ad7490_adc;
	struct iio_dev *indio_dev;

	spi->bits_per_word = 12;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*ad7490_adc));
	if (!indio_dev) {
		dev_err(dev, "can not allocate iio device\n");
		return -ENOMEM;
	}

	indio_dev->dev.parent = dev;
	indio_dev->dev.of_node = dev->of_node;
	indio_dev->info = &ad7490_spi_info;
	indio_dev->name = spi_get_device_id(spi)->name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = 
}

int ad7490_spi_remove(struct spi_device *spi)
{

}

void ad7490_spi_shutdown(struct spi_device *spi)
{

}

static const struct of_device_id ad7490_of_id[] {
	{ .compatable = "adi,ad7490" },
	{}
};
MODULE_DEVICE_TABLE(of, ad7490_of_id);

static const struct spi_device_id ad7490_spi_id[] {
	{ "ad7490", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, ad7490_spi_id);

static struct spi_driver ad7490_spi_driver = {
	.id_table = ad7490_spi_id,
	.probe    = ad7490_spi_probe,
	.remove   = ad7490_spi_remove,
	.shutdown = ad7490_spi_shutdown,
	.driver   = {
		.name 		= "ad7490",
		.of_match_table = ad7490_of_id,
	},
};
module_spi_driver(ad7490_spi_driver);

MODULE_AUTHOR("Byron Lathi <bslathi19@gmail.com>");
MODULE_DESCRIPTION("Analog Devices 12-bit 16-channel ADC driver");
MODULE_LICENSE("GPL v2");
