#ifndef ___KLCIPH_H__
#define ___KLCIPH_H__

Ret klciph_init();
void klciph_deinit();

//
// Just for libccll
#define cl_klciph_init() \
	klciph_init()
#define cl_klciph_deinit() \
	klciph_deinit()
// libccll -end-

#endif
