#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

struct ad7490_chip {

}

static int ad7490_spi_probe(struct spi_device *spi)
{
	struct ad7490_chip

	printk(KERN_INFO "AD7490 probe\n");
	return 0;
}

static int ad7490_spi_remove(struct spi_device *spi)
{
	printk(KERN_INFO "AD7490 probe\n");
	return 0;
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
	.remove = ad7490_spi_remove,
	.id_table = ad7490_spi_id,
};
module_spi_driver(ad7490_spi_driver);

MODULE_AUTHOR("Byron Lathi <bslathi19@gmail.com>");
MODULE_DESCRIPTION("Analog Devices 12-bit 16-channel ADC driver");
MODULE_LICENSE("GPL v2");
