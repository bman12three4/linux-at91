#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct ad7490_adc_data {
	struct spi_device *spi;
	struct mutex lock;
};

static int ad7490_spi_read_channel(struct ad7490_adc_data *ad7490_adc, int* val,
				unsigned int channel)
{
		int tx = 0x4141;

		struct spi_transfer	t = {
			.rx_buf		= val,
			.tx_buf		= &tx,
			.len		= 16,
		};

		return spi_sync_transfer(ad7490_adc->spi, &t, 1);
}

#define AD7490_ADC_CHANNEL(chan) {				\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = (chan),					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
}

static const struct iio_chan_spec ad7490_channels[] = {
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

static int ad7490_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	struct ad7490_adc_data *ad7490_adc = iio_priv(indio_dev);
	int ret;

	if (!val)
		return -EINVAL;

	switch (mask){
	case (IIO_CHAN_INFO_RAW):
		mutex_lock(&ad7490_adc->lock);
		ret = ad7490_spi_read_channel(ad7490_adc, val, chan->channel);
		mutex_unlock(&ad7490_adc->lock);

		if (ret < 0)
			printk(KERN_INFO "AD7490 SPI Read error\n");

		return IIO_VAL_INT;

	case (IIO_CHAN_INFO_SCALE):
		*val = 2048;
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static const struct iio_info ad7490_info = {
	.read_raw = ad7490_read_raw,
};

static int ad7490_spi_probe(struct spi_device *spi)
{
	struct ad7490_adc_data *ad7490_adc;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*ad7490_adc));
	if (!indio_dev)
		return -ENOMEM;

	ad7490_adc = iio_priv(indio_dev);
	spi_set_drvdata(spi, indio_dev);
	ad7490_adc->spi = spi;

	indio_dev->dev.parent = &spi->dev;
	indio_dev->info = &ad7490_info;
	indio_dev->name = "ad7490";
	indio_dev->channels = ad7490_channels;
	indio_dev->num_channels = 16;
	indio_dev->modes = INDIO_DIRECT_MODE;

	mutex_init(&ad7490_adc->lock);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static const struct of_device_id ad7490_dt_ids[] = {
	{
		.compatible = "adi,ad7490",
	},
	{}
};
MODULE_DEVICE_TABLE(of, ad7490_dt_ids);

static const struct spi_device_id ad7490_spi_id[] = {
	{ "ad7490", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, ad7490_spi_id);

static struct spi_driver ad7490_spi_driver = {
	.driver = {
		.name = "ad7490",
		.of_match_table = ad7490_dt_ids,
	},
	.probe = ad7490_spi_probe,
	.id_table = ad7490_spi_id,
};
module_spi_driver(ad7490_spi_driver);

MODULE_AUTHOR("Byron Lathi <bslathi19@gmail.com>");
MODULE_DESCRIPTION("Analog Devices 12-bit 16-channel ADC driver");
MODULE_LICENSE("GPL v2");
