#ifndef INOUT_H
#define INOUT_H

#define CTRL_KEY(c) ((c) & 0x1f)

char editor_read_key(void);
void editor_process_key_press(void);

#endif