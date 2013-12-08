.686
.model flat,stdcall
option casemap:none

include C:\masm32\include\msvcrt.inc
includelib C:\masm32\lib\msvcrt.lib

.xmm

.data

global$_r	dd	1
global$_l	dd	0
global$_g	dd	0
global$_m	dq	4607182418800017408
global$_n	dq	0
global$_h	dq	0
doubleConst$_1	dq	4607182418800017408
stringConst$_1	db	"100! = %d", 10,	0
.code

f proc

param$_n0 = 8
func_ret = 12

push ebp
mov ebp, esp

lea eax, func_ret[ebp]
push eax
push dword ptr param$_n0[ebp]
xor ebx, ebx
pop eax
xor ecx, ecx
cmp eax, ebx
sete cl
jz label$_1
push 1
jmp label$_0

label$_1:

push dword ptr param$_n0[ebp]
sub esp, 4
push dword ptr param$_n0[ebp]
mov ebx, 1
pop eax
sub eax, ebx
push eax
call f
add esp, 4
pop ebx
pop eax
imul eax, ebx
push eax

label$_0:

pop ebx
pop eax
mov dword ptr [eax], ebx



fRetLabel:

mov esp, ebp
pop ebp
ret 

f endp

main:

local$_x0 = -4
func_ret = 8

push ebp
mov ebp, esp
sub esp, 8

sub esp, 4
push 100
call f
add esp, 4
push offset stringConst$_1
call _imp__printf
add esp, 12

lea eax, func_ret[ebp]
push eax
xor ebx, ebx
pop eax
mov dword ptr [eax], ebx



mainRetLabel:

add esp, 4
mov esp, ebp
pop ebp
ret 

end main

