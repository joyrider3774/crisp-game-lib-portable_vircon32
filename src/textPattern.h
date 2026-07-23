#ifndef TEXT_PATTERN_H
#define TEXT_PATTERN_H

// Vircon32 port note: unlike standard C, this compiler treats a variable
// declaration in a header plus its definition in the matching .c file as
// a DUPLICATE declaration error (there's no linker to reconcile "extern"
// tentative declarations with the real one). Since this is a single
// translation unit anyway, textPattern.c's definition of "textPatterns"
// is the only declaration that exists - just make sure textPattern.c is
// #included before anything that references textPatterns (cglp.c).

#endif
