#ifndef AE_CONFIG_H_
#define AE_CONFIG_H_

struct ae_config {
	char *argz;
	size_t len;
};

int ae_config_read(struct ae_config *conf, const char *path);
int ae_config_read_from_string(struct ae_config *conf, const char *string);
const char *ae_config_get(const struct ae_config *conf, const char *key);
void ae_config_cleanup(struct ae_config *conf);

#endif /* AE_CONFIG_H_ */
