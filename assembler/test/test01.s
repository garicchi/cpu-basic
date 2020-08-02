ldh r0, 0x00
ldl r0, 0x00
ldh r1, 0x00
ldl r1, 0x01
ldh r2, 0x00
ldl r2, 0x00
ldh r3, 0x00
ldl r3, 0x0A
loop:
add r2, r1
add r0, r2
st r0, 0x64
cmp r2, r3
je else
jmp loop
else:
hlt
