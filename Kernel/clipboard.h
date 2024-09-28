#ifndef INCLUDED_CLIPBOARD_H
#define INCLUDED_CLIPBOARD_H

uintn Clip_Set(const ascii* data, uintn length);

uintn Clip_Get(out ascii* buff, in out uintn* buffsize);

#endif
