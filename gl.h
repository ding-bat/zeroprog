
#ifndef GL_H__
#define GL_H__    1

#define    GL_OK             0
#define    GL_ELIBUSB       -1
#define    GL_EOPEN         -2
#define    GL_ESETCONFIG    -3
#define    GL_ECLAIM        -4

int gl_gpio_oe(unsigned int mask);
int gl_gpio_read(void);
int gl_gpio_write(unsigned int val);

int gl_write_address(unsigned int address);
int gl_write_data(unsigned int val);
int gl_read_data(void);
int gl_read_bulk(void *buffer, unsigned int size);
int gl_reg_write(unsigned int reg, unsigned int val);
int gl_reg_read(unsigned int reg);
int gl_open(int vid);
int gl_close(void);

#endif
