/* Example C code for `map` using vectored returns and constructor variables
   read from the `Node` pointer.

STG:

    data List a = Cons a (List a) | Nil

    map = {} \n {f,xs} ->
      case xs of
        Nil {} -> Nil {}
        Cons {head,tail} ->
          let fHead    = {f,head} \u {} -> f {head}
              mapFTail = {f,tail} \u {} -> map {f,tail}
          in Cons {fHead,mapFTail}

     main = {} \n {} -> case map f xs of
       ...
 */
#include <stdlib.h>
#include <stdio.h>

/* alias for return, this could be replaced with a tailcall */
#define jump return

/* entering a closure jumps to the standard entry code */
#define enter(closure) jump(((void**) closure)[0])

/* the A stack manages the closure values */
void** a_stack;
void** a_stack_start;
void** a_stack_end;

size_t size_a(void) {
    return a_stack - a_stack_start;
}

void push_a(void* value) {
    if (a_stack == a_stack_end) {
        fputs("a_stack is full\n", stderr);
        abort();
    }

    ++a_stack;
    a_stack[0] = value;
}

void* peek_a(void) {
    if (a_stack == a_stack_start) {
        fputs("a_stack is empty\n", stderr);
        abort();
    }

    return a_stack[0];
}

void* pop_a(void) {
    if (a_stack == a_stack_start) {
        fputs("a_stack is empty\n", stderr);
        abort();
    }

    --a_stack;
    return a_stack[1];
}

/* the B stack manages the non-closure values, this includes unboxed values
   and continuation pointers */
void** b_stack;
void** b_stack_start;
void** b_stack_end;

size_t size_b(void) {
    return b_stack - b_stack_start;
}

void push_b(void* value) {
    if (b_stack == b_stack_end) {
        fputs("b_stack is full\n", stderr);
        abort();
    }

    b_stack[0] = value;
    ++a_stack;
}

void* peek_b(void) {
        if (b_stack == b_stack_start) {
        fputs("b_stack is empty\n", stderr);
        abort();
    }

    return b_stack[0];
}

void* pop_b(void) {
    if (b_stack == b_stack_start) {
        fputs("b_stack is empty\n", stderr);
        abort();
    }

    --b_stack;
    return b_stack[1];
}

/* the heap as an array of closures */
void** heap;

/* the currently executing closure */
void* node;

/* registers to hold small results for constructors */
void* return_register_1;
void* return_register_2;

/* the entry code for a cons cell */
void* cons_entry_code(void) {
    /* source:

       Cons a (List a)
    */

    void** return_vector = pop_b();
    return_register_1 = ((void**) node)[1];
    return_register_2 = ((void**) node)[2];
    jump(return_vector[0]);
}

void* cons_info_table[] = {cons_entry_code};

/* the entry code for an empty list */
void* nil_entry_code(void) {
    /* source:

       Nil
    */

    void** return_vector = pop_b();
    jump(return_vector[1]);
}

void* nil_info_table[] = {nil_entry_code};

/* forward declare map_direct */
void* map_direct(void);

/* the entry code for the `map` top level binding */
void* map_standard_entry(void) {
    /* source:

       map {} \n {f,xs} ->
         case xs of
           Nil {} -> Nil {}
           Cons {head,tail} ->
             let fHead    = {f,head} \u {} -> f {head}
                 mapFTail = {f,tail} \u {} -> map {f,tail}
             in Cons {fHead,mapFTail}
    */

    if (size_a() < 2) {
        /* invoke update code */
    }

    jump(map_direct);
}

/* the continuation for the `Nil` alternative in the `map` function */
void* map_return_nil(void) {
    /* source:

       Nil {} -> Nil {}
    */

    /* pop arguments to map */
    pop_a();
    pop_a();

    /* return to the next return vector's nil return */
    jump(nil_entry_code);
}

/* the entry code for the `fHead` lambda-form in the `Cons` alternative of the
   `map` function */
void* f_head_entry_code(void) {
    /* source:

       fHead = {f,head} \u {} -> f {head}
    */

    /* we should push update frame but that is getting too complicated*/

    /* push `head` */
    push_a(((void**) node)[2]);

    /* set `node` to `f` */
    node = ((void**) node)[1];

    enter(node);
}

void* f_head_info_table[] = {f_head_entry_code};

/* the entry code for the `mapFTail` lambda-form in the `Cons` alternative of
   the `map` function */
void* map_f_tail_entry_code(void) {
    /* source:

       mapFTail = {f,tail} \u {} -> map {f,tail}
    */

    /* we should push update frame but that is getting too complicated*/

    /* push `f`  */
    push_a(((void**) node)[1]);
    /* push `tail` */
    push_a(((void**) node)[2]);

    /* we know the direct call map because it is a top-level binding */
    jump(map_direct);
}

void* map_f_tail_info_table[] = {map_f_tail_entry_code};

/* the continuation for the `Cons` alternative in the `map` function */
void* map_return_cons(void) {
    /* source:

       Cons {head,tail} ->
         let fHead    = {f,head} \u {} -> f {head}
             mapFTail = {f,tail} \u {} -> map {f,tail}
         in Cons {fHead,mapFTail}
    */

    /* initialize `fHead` */
    heap[1] = f_head_info_table;
    heap[2] = a_stack[-1];  /* f */
    heap[3] = return_register_1;  /* head */

    return_register_1 = &heap[1];

    /* initialize `mapFTail */
    heap[4] = map_f_tail_info_table;
    heap[5] = a_stack[-1];  /* f */
    heap[6] = return_register_2;  /* tail */

    return_register_2 = &heap[4];;

    /* allocate space for `fHead`, `mapFTail`, and the returned `Cons` cell */
    heap += 6;

    /* pop arguments to map */
    pop_a();
    pop_a();

    /* return to the next return vector's cons return */
    void** return_vector = pop_b();
    jump(return_vector[0]);
}

void* map_return_vector[] = {map_return_cons, map_return_nil};

/* the direct call site for the top-level `map` binding */
void* map_direct(void) {
    /* source:

       map {} \n {f,xs} ->
         case xs of
           Nil {} -> Nil {}
           Cons {head,tail} ->
             let fHead    = {f,head} \u {} -> f {head}
                 mapFTail = {f,tail} \u {} -> map {f,tail}
             in Cons {fHead,mapFTail}
    */

    push_b(map_return_vector);
    node = peek_a();
    enter(node);
}

void* map_info_table[] = {map_standard_entry};

/* the small interpreter for evaluating code without blowing up the C stack */
void evaluate(void* closure) {
    node = closure;
    void* (*cont)(void) = *((void**) closure);
    while (1) {
        cont = cont();
    }
}
