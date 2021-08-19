#include <stdlib.h>
#include "include/codegen.h"
#include "include/symtab.h"
#include "include/error.h"
#include "include/globals.h"


struct dregs regs, *d_regs = &regs;

void push(int value)
{
    if(d_regs->cgen_stack.top == d_regs->cgen_stack.size)
    {
        d_regs->cgen_stack.size += 1024;
        if(realloc(d_regs->cgen_stack.data, d_regs->cgen_stack.size) == NULL)
        {
            fprintf(stderr, "realloc() failed: error in stack.push() during code generation\n");
            exit(-1);   
        }
    }
    if(d_regs->cgen_stack.top < 0)
        d_regs->cgen_stack.top = 0;
    d_regs->cgen_stack.data[d_regs->cgen_stack.top++] = value;
}

int pop()
{
    int value = d_regs->cgen_stack.data[--d_regs->cgen_stack.top];
    return value;
}

void init_cgen_regs()
{
    d_regs->in_fpu_mode = 0;
    d_regs->st0.busy = d_regs->eax.busy = 0;
    d_regs->cgen_stack.size = 1024;
    d_regs->cgen_stack.data = alloc_mem(d_regs->cgen_stack.size);
    d_regs->cgen_stack.top = 0;
    d_regs->cgen_stack.push = &push;
    d_regs->cgen_stack.pop =  &pop;

}

void emit_cond_pop_eax()
{
    if(d_regs->st0.busy)
    {
        emitcode("sub esp, 4");
        emitcode("fstp dword [esp]");
        emitcode("pop eax");
        d_regs->eax.vartype = FLOAT_TYPE;
        d_regs->eax.busy = 1;
        d_regs->st0.busy = 0;
        return;
    }
    if(!d_regs->eax.busy)
    {
        emitcode("pop eax");
        d_regs->eax.vartype = d_regs->cgen_stack.pop();
        d_regs->eax.busy = 1;
    }
}

void emit_cond_push_eax()
{
    if(d_regs->st0.busy)
    {
        emitcode("sub esp, 4");
        emitcode("fstp dword [esp]");
        d_regs->st0.busy = 0;
        d_regs->cgen_stack.push(FLOAT_TYPE);
    }
    else if(d_regs->eax.busy)
    {
        emitcode("push eax");
        d_regs->cgen_stack.push(d_regs->eax.vartype);
        d_regs->eax.busy = 0;
    }
    
}

void emit_push_const(unsigned int value, int type)
{
    emitcode("push 0x%x", value); 
    d_regs->cgen_stack.push(type);
}

void emit_add()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
            
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");

            emitcode("add esp, 8");
            emitcode("faddp st1");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");

        emitcode("add esp, 4");
        emitcode("faddp st1");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("add eax, edx");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("add eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_sub()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");

            emitcode("add esp, 8");
            emitcode("fxch");
            emitcode("fsubp st1");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");

        emitcode("add esp, 4");
        emitcode("fxch");
        emitcode("fsubp st1");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("sub eax, edx");

            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xchg eax, edx");
        emitcode("sub eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_mul()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
    
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");

            emitcode("add esp, 8");
            emitcode("fmulp st1");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");

        emitcode("add esp, 4");
        emitcode("fmulp st1");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("imul eax, edx");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("imul eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_div()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");

            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
    
            emitcode("add esp, 8");
            emitcode("fxch");
            emitcode("fdivp st1");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");

        emitcode("add esp, 4");
        emitcode("fxch");
        emitcode("fdivp st1");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop ebx");
            emitcode("pop eax");
            emitcode("cdq");
            emitcode("idiv ebx");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop ebx");
        emitcode("xchg eax, ebx");
        emitcode("cdq");
        emitcode("idiv ebx");
        d_regs->cgen_stack.pop();
    }
}
void emit_mod()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop ebx");
            emitcode("pop eax");
            emitcode("cdq");
            emitcode("idiv ebx");
            emitcode("mov eax, edx");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop ebx");
        emitcode("xchg eax, ebx");
        emitcode("cdq");
        emitcode("idiv ebx");
        emitcode("mov eax, edx");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop ebx");
            emitcode("pop eax");
            emitcode("cdq");
            emitcode("idiv ebx");
            emitcode("mov eax, edx");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop ebx");
        emitcode("xchg eax, ebx");
        emitcode("cdq");
        emitcode("idiv ebx");
        emitcode("mov eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_neg()
{
    if(d_regs->in_fpu_mode)
    {
        if(d_regs->st0.busy)
        {
            emitcode("fchs st0");
            return;
        }
    
        emitcode("fld dword [esp]");
        emitcode("fchs");
        emitcode("fstp dword [esp]");   
            
    }
    else
    {
        if(d_regs->eax.busy)
        {
            emitcode("neg eax");
            return;
        }
        emitcode("pop eax");
        emitcode("neg eax");
        emitcode("push eax");
    }
}


void emit_not()
{
    if(d_regs->in_fpu_mode)
    {
        if(d_regs->st0.busy)
        {
            emitcode("fstp [esp-4]");
            emitcode("mov eax, [esp-4]");
            emitcode("cmp eax, 0");
            emitcode("sete al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }

        emitcode("mov eax, [esp]");
        emitcode("cmp eax, 0");
        emitcode("sete al");
        emitcode("movzx eax, al");
        emitcode("mov [esp], eax");
        emitcode("fild dword [esp]");
        emitcode("fstp dword [esp]");   
            
    }
    else
    {
        if(d_regs->eax.busy)
        {
            emitcode("cmp eax, 0");
            emitcode("sete al");
            emitcode("movzx eax, al");
            return;
        }
        emitcode("pop eax");
        emitcode("cmp eax, 0");
        emitcode("sete al");
        emitcode("movzx eax, al");
        emitcode("push eax");
    }
}


void emit_flip()
{
    if(d_regs->in_fpu_mode)
    {
        if(d_regs->st0.busy)
        {
            emitcode("fistp dword [esp-4]");
            emitcode("mov eax, dword [esp-4]");
            emitcode("xor eax, 0xffffffff");
            emitcode("mov [esp-4], eax");
            emitcode("fild qword [esp-4]");
            return;
        }

        int type = d_regs->cgen_stack.pop();
        if(type == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop eax");
        emitcode("xor eax, 0xffffffff");
        emitcode("push eax");
        d_regs->cgen_stack.push(INT_TYPE);
    }
    else
    {
        if(d_regs->eax.busy)
        {
            emitcode("xor eax, 0xffffffff");
            return;
        }
        emitcode("pop eax");
        emitcode("xor eax, 0xffffffff");
        emitcode("push eax");
    }
}


void emit_bit_xor()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("xor eax, edx");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop edx");
        emitcode("xor eax, edx");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("xor eax, edx");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xor eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_bit_and()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("and eax, edx");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop edx");
        emitcode("and eax, edx");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("and eax, edx");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("and eax, edx");
        d_regs->cgen_stack.pop();
    }
}

void emit_bit_or()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("or eax, edx");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop edx");
        emitcode("or eax, edx");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("or eax, edx");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("or eax, edx");
        d_regs->cgen_stack.pop();
    }
}


void emit_bit_lshift()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop ecx");
            emitcode("pop eax");
            emitcode("shl eax, cl");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop ecx");
        emitcode("xchg eax, ecx");
        emitcode("shl eax, cl");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop ecx");
            emitcode("pop eax");
            emitcode("shl eax, cl");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop ecx");
        emitcode("xchg eax, ecx");
        emitcode("shl eax, cl");
        d_regs->cgen_stack.pop();
    }
}

void emit_bit_rshift()
{
   
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp]");
                emitcode("fistp dword [esp]");
            }
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            {
                emitcode("fld dword [esp+4]");
                emitcode("fistp dword [esp+4]");
            }
            emitcode("pop ecx");
            emitcode("pop eax");
            emitcode("shr eax, cl");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
       
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, dword [esp-4]");
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
        {
            emitcode("fld dword [esp]");
            emitcode("fistp dword [esp]");
        }
        emitcode("pop ecx");
        emitcode("xchg eax, ecx");
        emitcode("shr eax, cl");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        
    }
    else
    {

        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop ecx");
            emitcode("pop eax");
            emitcode("shr eax, cl");
        
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop ecx");
        emitcode("xchg eax, ecx");
        emitcode("shr eax, cl");
        d_regs->cgen_stack.pop();
    }
}


void emit_logical_and()
{
    // Works in both floating and fixed point mode becuase zero is '0x0' in both modes
    if(!d_regs->eax.busy)
    {
        d_regs->eax.busy = 1;
        emitcode("pop edx");
        emitcode("pop eax");
        emitcode("and eax, edx");
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
        
        d_regs->cgen_stack.pop();
        d_regs->cgen_stack.pop();
        return;
    }
    emitcode("pop edx");
    emitcode("and eax, edx");
    emitcode("cmp eax, 0");
    emitcode("setne al");
    emitcode("movzx eax, al");
    d_regs->cgen_stack.pop();
}

void emit_logical_or()
{
    // Works in both floating and fixed point mode becuase zero is '0x0' in both modes
    if(!d_regs->eax.busy)
    {
        d_regs->eax.busy = 1;
        emitcode("pop edx");
        emitcode("pop eax");
        emitcode("or eax, edx");
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
        
        d_regs->cgen_stack.pop();
        d_regs->cgen_stack.pop();
        return;
    }
    emitcode("pop edx");
    emitcode("or eax, edx");
    emitcode("cmp eax, 0");
    emitcode("setne al");
    emitcode("movzx eax, al");
    d_regs->cgen_stack.pop();
}

void emit_equ()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
    
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x40");
            emitcode("cmp ecx, 0x40");
            emitcode("sete al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");  // dont want to overwite any data on stack
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
    
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x40");
        emitcode("cmp ecx, 0x40");
        emitcode("sete al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("sete al");
            emitcode("movzx eax, al");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("cmp eax, edx");
        emitcode("sete al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_not_equ()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");

            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
        
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x40");
            emitcode("cmp ecx, 0x40");
            emitcode("setne al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
    
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x40");
        emitcode("cmp ecx, 0x40");
        emitcode("setne al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("setne al");
            emitcode("movzx eax, al");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();

            return;
        }
        emitcode("pop edx");
        emitcode("cmp eax, edx");
        emitcode("setne al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_gtr()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
        
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x45");  // all bits must be zero
            emitcode("cmp ecx, 0");
            emitcode("sete al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
    
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x45");
        emitcode("cmp ecx, 0");
        emitcode("sete al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("setg al");
            emitcode("movzx eax, al");
            
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xchg eax, edx");
        emitcode("cmp eax, edx");
        emitcode("setg al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_less()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
    
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x1");
            emitcode("cmp ecx, 1");
            emitcode("sete al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
        
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x1");
        emitcode("cmp ecx, 1");
        emitcode("sete al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("setl al");
            emitcode("movzx eax, al");
            
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xchg eax, edx");
        emitcode("cmp eax, edx");
        emitcode("setl al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_gtr_equ()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
            
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x45");
            emitcode("cmp ecx, 0");
            emitcode("sete al");
            emitcode("movzx eax, al");
            emitcode("and ecx, 0x40");
            emitcode("cmp ecx, 0x40");
            emitcode("sete bl");
            emitcode("movzx ebx, bl");
            emitcode("or eax, ebx");    
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
    
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x45");
        emitcode("cmp ecx, 0x40");
        emitcode("setle al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("setge al");
            emitcode("movzx eax, al");
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xchg eax, edx");
        emitcode("cmp eax, edx");
        emitcode("setge al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_less_equ()
{
    if(d_regs->in_fpu_mode)
    {
        if(!d_regs->st0.busy)
        {
            d_regs->eax.busy = 0;
            d_regs->st0.busy = 1;
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp]");
            else
                emitcode("fild dword [esp]");
        
            if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
                emitcode("fld dword [esp+4]");
            else
                emitcode("fild dword [esp+4]");
        
            emitcode("add esp, 8");
            emitcode("fcomp st1");
            emitcode("fstsw ax");
            emitcode("fwait");
            emitcode("sahf");
            emitcode("pushf");
            emitcode("pop ecx");
            emitcode("and ecx, 0x41");
            emitcode("cmp ecx, 0");
            emitcode("setg al");
            emitcode("movzx eax, al");
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            return;
        }
        
        if(d_regs->cgen_stack.pop() == FLOAT_TYPE)
            emitcode("fld dword [esp]");
        else
            emitcode("fild dword [esp]");
        
        emitcode("add esp, 4");
        emitcode("fcomp st1");
        emitcode("fstsw ax");
        emitcode("fwait");
        emitcode("sahf");
        emitcode("pushf");
        emitcode("pop ecx");
        emitcode("and ecx, 0x41");
        emitcode("cmp ecx, 0");
        emitcode("setg al");
        emitcode("movzx eax, al");
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
    }
    else
    {
        if(!d_regs->eax.busy)
        {
            d_regs->eax.busy = 1;
            emitcode("pop edx");
            emitcode("pop eax");
            emitcode("cmp eax, edx");
            emitcode("setle al");
            emitcode("movzx eax, al");
           
            d_regs->cgen_stack.pop();
            d_regs->cgen_stack.pop();
            return;
        }
        emitcode("pop edx");
        emitcode("xchg eax, edx");
        emitcode("cmp eax, edx");
        emitcode("setle al");
        emitcode("movzx eax, al");
        d_regs->cgen_stack.pop();
    }
}

void emit_assign_to_variable(struct symbol *sym, int scope)
{
    if(d_regs->in_fpu_mode && sym->type != FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    else if(!d_regs->in_fpu_mode && sym->type == FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        emitcode("fstp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }

    if(scope == LOCAL_SCOPE)
    {
        if((sym->type == CHAR || sym->type == BOOL) && !sym->status.is_ptr_count)
            emitcode("mov [ebp%+d], al", sym->stack.offset);
        else
            emitcode("mov [ebp%+d], eax", sym->stack.offset); 
    }
    else
    {
        if((sym->type == CHAR || sym->type == BOOL) && !sym->status.is_ptr_count)
            emitcode("mov dword [%s], al", sym->name);
        else
            emitcode("mov dword [%s], eax", sym->name);
    }
  
}


void emit_assign_rodata_string(struct symbol *sym, unsigned int seg_offset, int scope)
{
    emitcode("lea eax, [char+%d]", seg_offset);
    if(scope == LOCAL_SCOPE)
        emitcode("mov [ebp%+d], eax", sym->stack.offset);
    else
        emitcode("mov [%s], eax", sym->name);
}


void emit_assign_rodata_string_to_pointer(unsigned int seg_offset)
{
    emitcode("lea eax, [char+%d]", seg_offset);
    emitcode("mov [edi], eax");
}

void emit_assign_to_pointer(int is_byte)
{
    if(is_byte)
        emitcode("mov [edi], al");
    else
        emitcode("mov [edi], eax");
}

// Eg: char *strs[] = {"abc", "def"}
void emit_assign_rodata_string_init_array(struct symbol *sym, int index, unsigned int seg_offset, int scope)
{
    if(scope == LOCAL_SCOPE)
        emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    else 
        emitcode("mov edi, [%s]", sym->name);

    emitcode("lea eax, [char+%d]", seg_offset);
    emitcode("mov [edi%+d], eax", index*4);
}



void emit_assign_member_rodata_string(unsigned int seg_offset)
{
    emitcode("lea eax, [char+%d]", seg_offset);
    emitcode("mov [edi], eax");
}


void emit_push_variable(struct symbol *sym, int scope)
{
    if(scope == LOCAL_SCOPE)
        emitcode("mov eax, dword [ebp%+d]", sym->stack.offset);
    else
        emitcode("mov eax, dword [%s]", sym->name);

    if(sym->type == CHAR  && !sym->status.is_ptr_count && !sym->status.is_array)
        emitcode("movzx eax, al");
    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }
    emitcode("push eax");

    if(sym->type == FLOAT)
    {
        d_regs->in_fpu_mode = 1;
        d_regs->cgen_stack.push(FLOAT_TYPE);
    }
    else
    {
        d_regs->cgen_stack.push(INT_TYPE);
    }
}


void emit_deref_variable(struct symbol *sym, int dref_count, int is_byte)
{
    emitcode("pop eax");

    while(dref_count)
    {
        if(dref_count == 1 && is_byte)
        {
            emitcode("mov eax, [eax]");
            emitcode("movzx eax, al");
            break;
        }
    
        emitcode("mov eax, [eax]");
        dref_count--;
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }
    emitcode("push eax");
    
    if(sym->type == FLOAT)
    {
        d_regs->in_fpu_mode = 1;
        d_regs->cgen_stack.push(FLOAT_TYPE);
    }
    else
    {
        d_regs->cgen_stack.push(INT_TYPE);
    }
    
}


void emit_deref_get_addr(struct symbol *sym, int dref_count)
{
    emitcode("pop eax");
    while(dref_count)
    {
        if(dref_count == 1)
        {
            emitcode("lea edi, [eax]");
            return;
        }
    
        emitcode("mov eax, [eax]");
        dref_count--;
    }
}

void emit_push_addrof_variable(struct symbol *sym, int scope)
{

    if(sym->status.is_array)
    {
        if(scope == LOCAL_SCOPE)
            emitcode("mov eax, [ebp%+d]", sym->stack.offset);
        else
            emitcode("mov eax, [%s]", sym->name);
    }
    else
    {
        if(scope == LOCAL_SCOPE)
            emitcode("lea eax, [ebp%+d]", sym->stack.offset);
        else
            emitcode("lea eax, [%s]", sym->name);
    }

    emitcode("push eax");
    d_regs->cgen_stack.push(INT_TYPE);
}


void emit_buffer_string(struct symbol *sym, char *string)  //eg: char buf[] = "This"
{
    // Note: endianess and stack grows from higher to lower addresses
    int val, len = strlen(string);
    char *ptr = string;

    
    int offset = 0;
    emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    for(int i = 0; i < (len/4); i++)
    {
        val = 0;
        val |= *ptr++;
        val |= (*ptr++ << 8);
        val |= (*ptr++ << 16);
        val |= (*ptr++ << 24);

        emitcode("mov dword [edi+%d], 0x%x", offset, val);
        offset += 4;
        
    }

    if(*ptr == 0)
    {
        emitcode("mov dword [edi+%d], 0x%x", offset, 0x00);
        return;
    }

    int shift = 0;
    val = 0;
    while(*ptr != '\0')
    {
        val |= (*ptr++ << shift);
        shift += 8;
    }
    emitcode("mov dword [edi+%d], 0x%x", offset, val);
}


void emit_array_init_element(struct symbol *sym, int index)
{
    if(d_regs->in_fpu_mode && sym->type != FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    if(!d_regs->in_fpu_mode && sym->type == FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        emitcode("fstp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }

    emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    if((sym->type == CHAR || sym->type == BOOL) && !sym->status.is_ptr_count)
        emitcode("mov [edi%+d], al", index);
    else
        emitcode("mov [edi%+d], eax", (index * 4));
}

void emit_push_array_element(struct symbol *sym, int scope)
{
    if(scope == LOCAL_SCOPE)
        emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    else
        emitcode("mov edi, [%s]", sym->name);

    if((sym->type == CHAR || sym->type == BOOL) && ((sym->status.is_array && !sym->status.is_ptr_count) || (sym->status.is_ptr_count == 1 && !sym->status.is_array)))
    {
        emitcode("mov al, byte [edi+eax]");
        emitcode("movzx eax, al");
    }
    else
    {
        emitcode("imul eax, 4");
        emitcode("mov eax, [edi+eax]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }
    emitcode("push eax");

    if(sym->type == FLOAT)
    {
        d_regs->in_fpu_mode = 1;
        d_regs->cgen_stack.push(FLOAT_TYPE);
    }
    else
    {
        d_regs->cgen_stack.push(INT_TYPE);
    }
}

void emit_push_addrof_array_element(struct symbol *sym, int scope)
{
    if(scope == LOCAL_SCOPE)
        emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    else
        emitcode("mov edi, [%s]", sym->name);

    if((sym->type == CHAR || sym->type == BOOL) && ((sym->status.is_array && !sym->status.is_ptr_count) || (sym->status.is_ptr_count == 1 && !sym->status.is_array)))
    {
        emitcode("lea eax, [edi+eax]");
    }
    else
    {
        emitcode("imul eax, 4");
        emitcode("lea eax, [edi+eax]");
    }
    emitcode("push eax");
    d_regs->cgen_stack.push(INT_TYPE);
}


void emit_adjust_variable_array(struct symbol *sym, int scope)
{
    if(d_regs->in_fpu_mode)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    
    if(scope == LOCAL_SCOPE)
        emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    else
        emitcode("mov edi, [%s]", sym->name);

    if(!((sym->type == CHAR || sym->type == BOOL) && sym->status.is_array && !sym->status.is_ptr_count) && !(sym->type == CHAR && sym->status.is_ptr_count == 1 && !sym->status.is_array))
        emitcode("imul eax, 4");

    emitcode("add edi, eax");
}

void emit_assign_rodata_string_to_index(unsigned int seg_offset)
{
    emitcode("lea eax, [char+%d]", seg_offset);
    emitcode("mov [edi], eax");
}

void emit_assign_to_array_index(struct symbol *sym)
{

    if(d_regs->in_fpu_mode && sym->type != FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    if(!d_regs->in_fpu_mode && sym->type == FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        emitcode("fstp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }

    if((sym->type == CHAR || sym->type == BOOL) && ((sym->status.is_array && !sym->status.is_ptr_count) || (sym->status.is_ptr_count == 1 && !sym->status.is_array)))
        emitcode("mov [edi], al");
    else
        emitcode("mov [edi], eax");
}

void emit_assign_block_memory(struct symbol *sym, int scope)
{
    int offset = 0;
    if(scope == GLOBAL_SCOPE)
    {
        offset = 4;
        emitcode("lea edx, [%s+%d]", sym->name, offset);
        emitcode("mov [%s], edx", sym->name);
        return;
    }

    if(sym->status.is_array)
    {
        if(sym->type == STRUCT && !sym->status.is_ptr_count)
            offset = sym->defn.array.size * sym->defn.structure.defn_ptr->size;
        else if(sym->type == CHAR && !sym->status.is_ptr_count)
            offset = sym->defn.array.size;
        else
            offset = sym->defn.array.size * 4;

    }
    else
    {
        offset = sym->defn.structure.defn_ptr->size;
    }

    emitcode("lea edx, [ebp%+d]", sym->stack.offset - offset);
    emitcode("mov [ebp%+d], edx", sym->stack.offset);
}

void emit_struct_address(struct symbol *sym, int scope)
{
    if(sym->status.is_ptr_count && !sym->status.is_array)
    {
        if(scope == LOCAL_SCOPE)
            emitcode("lea edi, [ebp%+d]", sym->stack.offset);
        else 
            emitcode("lea edi, [%s]", sym->name);
        return;
    }

    if(scope == LOCAL_SCOPE)
        emitcode("mov edi, [ebp%+d]", sym->stack.offset);
    else
        emitcode("mov edi, [%s]", sym->name);
}

void emit_push_struct_address()
{
    emitcode("mov eax, edi");
    emitcode("push eax");
    d_regs->cgen_stack.push(INT_TYPE);
}

void emit_push_struct_value(struct symbol *sym, int is_byte)
{
    if(is_byte)
    {
        emitcode("xor eax, eax");
        emitcode("mov al, byte [edi]");
    }
    else
    {
        emitcode("mov eax, [edi]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }
    emitcode("push eax");
    if(sym->type == FLOAT)
    {
        d_regs->in_fpu_mode = 1;
        d_regs->cgen_stack.push(FLOAT_TYPE);
    }
    else
    {
        d_regs->cgen_stack.push(INT_TYPE);
    }
}


void emit_reset_member_pointer()
{
    emitcode("mov edi, [edi]");
}

void emit_load_member_address(struct symbol *sym)
{
    emitcode("lea edi, [edi%+d]", sym->stack.offset);
}

void emit_adjust_struct_array(struct symbol *sym)
{
    if(d_regs->in_fpu_mode)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

    if(sym->type == STRUCT)
    {
        if(sym->status.is_ptr_count)
            emitcode("imul eax, 4");
        else
            emitcode("imul eax, %d", sym->defn.structure.defn_ptr->size);
    }
    else
    {
        if(!((sym->type == CHAR || sym->type == BOOL) && sym->status.is_array && !sym->status.is_ptr_count) && !((sym->type == CHAR || sym->type == BOOL) && sym->status.is_ptr_count == 1 && !sym->status.is_array))
            emitcode("imul eax, 4");
    }
    emitcode("add edi, eax");
}


void emit_assign_to_member(struct symbol *sym)
{
    if(d_regs->in_fpu_mode && sym->type != FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    if(!d_regs->in_fpu_mode && sym->type == FLOAT)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        emitcode("fstp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

    if(sym->type == BOOL)
    {
        emitcode("cmp eax, 0");
        emitcode("setne al");
        emitcode("movzx eax, al");
    }

    if((sym->type == CHAR || sym->type == BOOL) && !sym->status.is_ptr_count)
        emitcode("mov [edi], al");
    else
        emitcode("mov [edi], eax");
}

void emit_zero_cmp_eax()
{
    emitcode("cmp eax, 0");
}

void emit_jeq(char *label)
{
    emitcode("je %s", label);
}
void emit_jneq(char *label)
{
    emitcode("jne %s", label);
}
void emit_store_condition()
{
    emitcode("push eax");
}

void emit_load_condition()
{
    emitcode("pop eax");
}

void emit_jmp(char *label)
{
    emitcode("jmp %s", label);
}

void emit_call(char *label)
{
    emitcode("call _%s", label);
}

void emit_lib_call(char *label)
{
    emitcode("call %s", label);
}

void emit_sub_esp(int size)
{
    emitcode("sub esp, %d", size);
}

void emit_add_esp(int size)
{
    emitcode("add esp, %d", size);
}

void emit_func_prolog(int size)
{
    emitcode("push ebp");
    emitcode("mov ebp, esp");

    if(size)
    {
        if((size % 4) != 0)
            size += (4-(size % 4)); // roundup to the closest 4bytes
        emitcode("sub esp, %d", size);
    }
}

void emit_func_epilog()
{
    emitcode("leave");
    emitcode("ret");
}

void emit_push_func_arg()
{
    emitcode("push eax");
}


void emit_push_func_retval()
{
    emitcode("push eax");
    if(d_regs->in_fpu_mode)
        d_regs->cgen_stack.push(FLOAT_TYPE);
    else
        d_regs->cgen_stack.push(INT_TYPE);
    d_regs->eax.busy = 0;
}

void emit_push_func_float_arg()
{
    emitcode("mov [esp-4], eax");
    emitcode("fld dword [esp-4]");
    emitcode("sub esp, 8");
    emitcode("fstp qword [esp]");
}

void emit_rodata_func_string_arg(unsigned int seg_offset)
{
    emitcode("lea eax, [char+%d]", seg_offset);
}



void emit_struct_copy(struct symbol *sym, int safe_mem)
{
    // 'safe_mem' is necessary
    int size = sym->defn.structure.defn_ptr->size;
    alloca_count++;

    emitcode("sub esp, %d", safe_mem);
    emitcode("push eax");   // previously called cgen_expr(); storing return value
    emitcode("push %d", size);
    emit_lib_call("alloca");
    emitcode("add esp, 4");
    emitcode("pop edx");

    emitcode("push %d", size);
    emitcode("push edx");
    emitcode("push eax");   // allocated stack memory
    emit_lib_call("memcpy");
    emitcode("add esp, 12");
    emitcode("add esp, %d", safe_mem);
}

void emit_load_sym_addr(struct symbol *sym, int scope)
{
    if(scope == LOCAL_SCOPE)
        emitcode("lea eax, [ebp%+d]", sym->stack.offset); 
    else
        emitcode("lea eax, [%s]", sym->name);
}

void emit_return(struct symbol *func)
{
    enum rettype rtype = func->defn.function.ret_type;
    if(rtype)
    {
        if(d_regs->in_fpu_mode && rtype != RET_FLOAT)
        {
            emitcode("mov [esp-4], eax");
            emitcode("fld dword [esp-4]");
            emitcode("fistp dword [esp-4]");
            emitcode("mov eax, [esp-4]");
        }
        else if(!d_regs->in_fpu_mode && rtype == RET_FLOAT)
        {
            emitcode("mov [esp-4], eax");
            emitcode("fild dword [esp-4]");
            emitcode("fstp dword [esp-4]");
            emitcode("mov eax, [esp-4]");
        }

        if(rtype == RET_CHAR)
        {
            emitcode("movzx eax, al");
        }
        else if(rtype == RET_BOOL)
        {
            emitcode("cmp eax, 0");
            emitcode("setne al");
            emitcode("movzx eax, al");
        }
    }
    emitcode("leave");
    emitcode("ret");
}

void emit_return_rodata_string(unsigned int seg_offset)
{
    emitcode("lea eax, [char+%d]", seg_offset);
}

void emit_push_all_regs()
{
    emitcode("pushad");
}

void emit_pop_all_regs()
{
    // replace eax with the return address of the previously called function
    emitcode("mov [esp+28], eax");
    emitcode("popad");
}

void emit_push_edi_reg()
{
    emitcode("push edi");
}

void emit_pop_edi_reg()
{
    emitcode("pop edi");
}

void emit_lib_call_st0_to_eax()
{
    emitcode("fstp dword [esp-4]");
    emitcode("mov eax, [esp-4]");
}

void emit_param_cast(int type)
{
    if(type == FLOAT_TYPE)
    {
        emitcode("mov [esp-4], eax");
        emitcode("fild dword [esp-4]");
        emitcode("fstp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }
    else 
    {
        emitcode("mov [esp-4], eax");
        emitcode("fld dword [esp-4]");
        emitcode("fistp dword [esp-4]");
        emitcode("mov eax, [esp-4]");
    }

}