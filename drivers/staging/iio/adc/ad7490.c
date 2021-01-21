#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct ad7490_data {
	struct spi_device *spi;
};

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
	
}

static const struct iio_info ad7490_info = {
	.read_raw = ad7490_read_raw,
};

static int ad7490_spi_probe(struct spi_device *spi)
{
	struct ad7490_data *data;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	spi_set_drvdata(spi, indio_dev);
	data->spi = spi;

	indio_dev->dev.parent = &spi->dev;
	indio_dev->info = &ad7490_info;
	indio_dev->name = "ad7490";
	indio_dev->channels = ad7490_channels;
	indio_dev->num_channels = 16;
	indio_dev->modes = INDIO_DIRECT_MODE;

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
