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

	printk(KERN_INFO "ad7490: Writing to ad7490: %x\n", val);
	printk(KERN_INFO "ad7490: Current control word: %x\n", ad7490_adc->ctrl);

	ad7490_adc->ctrl = (val & mask) | (ad7490_adc->ctrl & ~mask);
	ad7490_adc->buffer = ad7490_adc->ctrl;
	spi_message_init_with_transfers(&msg, tx, 1);
	ret = spi_sync(ad7490_adc->spi, &msg);

	return ret;
}

static int ad7490_spi_read_channel(struct ad7490_adc_chip *ad7490_adc, int* val,
				unsigned int channel)
{
	int ret;
	int mask = GENMASK(12, 0);
	struct spi_message msg;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &ad7490_adc->ctrl,
			.rx_buf = &ad7490_adc->buffer,
			.len = 4,
		},
	};

	ret = ad7490_spi_write_ctrl(ad7490_adc,
					channel << AD7490_CHANNEL_OFFSET,
					AD7490_MASK_CHANNEL_SEL);

	if (ret) {
		printk(KERN_INFO "ad7490: Failed to write control word %d\n", ret);
		return ret;
	}

	ad7490_adc->buffer = 0;
	spi_message_init_with_transfers(&msg, xfer, 1);
	ret = spi_sync(ad7490_adc->spi, &msg);
	if (ret) {
		printk(KERN_INFO "ad7490: SPI Data transfer error %d\n", ret);
		return ret;
	}

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

static const struct iio_chan_spec ad7490_adc_channels[] = {
	AD7490_ADC_CHANNEL(0),
	AD7490_ADC_CHANNEL(1),
	AD7490_ADC_CHANNEL(2),
	AD7490_ADC_CHANNEL(3),
	AD7490_ADC_CHANNEL(4),
	AD7490_ADC_CHANNEL(5),
	AD7490_ADC_CHANNEL(6),
	AD7490_ADC_CHANNEL(7),
	AD7490_ADC_CHANNEL(8),
	AD7490_ADC_CHANNEL(9),
	AD7490_ADC_CHANNEL(10),
	AD7490_ADC_CHANNEL(11),
	AD7490_ADC_CHANNEL(12),
	AD7490_ADC_CHANNEL(13),
	AD7490_ADC_CHANNEL(14),
	AD7490_ADC_CHANNEL(15),
};

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
		mutex_unlock(&ad7490_adc->lock);

		if (ret < 0){
			printk(KERN_INFO "ad7490: Error reading from channel %d\n", ret);
			return ret;
		}

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

static int ad7490_spi_init(struct ad7490_adc_chip *ad7490_adc)
{
	int ret;

	ad7490_adc->current_channel = 0;
	ad7490_spi_write_ctrl(ad7490_adc, 0xffff, AD7490_MASK_TOTAL);
	ad7490_spi_write_ctrl(ad7490_adc, 0xffff, AD7490_MASK_TOTAL);

	// Normal power mode, sequencer off, double range, straight binary
	ret = ad7490_spi_write_ctrl(ad7490_adc, 0x0831, AD7490_MASK_TOTAL);

	return ret;
}

int ad7490_spi_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct ad7490_adc_chip *ad7490_adc;
	struct iio_dev *indio_dev;
	int ret;

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
	indio_dev->channels = ad7490_adc_channels;
	spi_set_drvdata(spi, indio_dev);

	ad7490_adc = iio_priv(indio_dev);
	ad7490_adc->indio_dev = indio_dev;
	ad7490_adc->spi = spi;
	indio_dev->num_channels = 16;

	mutex_init(&ad7490_adc->lock);

	ret = ad7490_spi_init(ad7490_adc);
	if (ret) {
		dev_err(dev, "unable to init this device: %d\n", ret);
		mutex_destroy(&ad7490_adc->lock);
		return ret;
	}

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "failed to register this device: %d\n", ret);
		mutex_destroy(&ad7490_adc->lock);
		return ret;
	}

	return 0;
	
}

int ad7490_spi_remove(struct spi_device *spi)
{
	struct iio_dev *indio_dev = spi_get_drvdata(spi);
	struct ad7490_adc_chip *ad7490_adc = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);
	mutex_destroy(&ad7490_adc->lock);

	return 0;
}

void ad7490_spi_shutdown(struct spi_device *spi)
{
	printk(KERN_ALERT "in function %s", __FUNCTION__);
}

static const struct of_device_id ad7490_of_id[] = {
	{ .compatible = "adi,ad7490" },
	{}
};
MODULE_DEVICE_TABLE(of, ad7490_of_id);

static const struct spi_device_id ad7490_spi_id[] = {
	{ "ad7490", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, ad7490_spi_id);

static struct spi_driver ad7490_spi_driver = {
	.id_table = ad7490_spi_id,
	.probe    = ad7490_spi_probe,
	.remove   = ad7490_spi_remove,
	//.shutdown = ad7490_spi_shutdown,
	.driver   = {
		.name 		= "ad7490",
		.of_match_table = ad7490_of_id,
	},
};
module_spi_driver(ad7490_spi_driver);

MODULE_AUTHOR("Byron Lathi <bslathi19@gmail.com>");
MODULE_DESCRIPTION("Analog Devices 12-bit 16-channel ADC driver");
MODULE_LICENSE("GPL v2");
