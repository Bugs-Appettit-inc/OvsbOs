# FASES 7–9 — libdrm / Mesa / integração: BLOQUEIO REAL (documentado)

## Tentado
- Verificada rede (lenta mas existe), userland OvsbOS e requisitos Mesa.

## Bloqueadores (arquiteturais, não contornáveis numa noite)
1. **Sem pthreads**: Gallium exige threads (`u_thread`, buffers, winsys).
   `src/userland/include/` não tem `pthread.h`/`threads.h`.
2. **Sem dlopen**: Mesa carrega drivers via `dlopen` (pipe-loader).
   Sem `dlfcn.h`, sem linker dinâmico no userland (tudo estático).
3. **Sem libwayland-client**: só existe protocolo raw nos testes; EGL
   precisa de plataforma EGL (wayland/surfaceless) completa.
4. **EXECBUFFER/CTX não implementados** (kernel retorna ENOSYS honesto
   com log `[DRM]`): sem Mesa para gerar o command stream, implementar
   às cegas é unverificável. Hand-craft de TGSI+submit estimado em >3h
   com baixa chance (30+ objetos de estado, sem feedback além de abort
   do renderer).

## O que FOI entregue no lugar (funcional e testado)
- `/dev/dri/card0` real: VERSION, GET_CAP, dumb create/map/destroy,
  GETRESOURCES/CRTC/CONNECTOR/ENCODER, ATOMIC no-op; `drmtest` PASS
  (version, caps, create, map+mmap+readback, destroy, resources).
- VIRTGPU_* retornam `-ENOSYS` com log (não mentem sucesso).

## Para desbloquear (ordem)
1. pthreads no userland (clone existe no kernel; falta a lib).
2. dlopen/linker dinâmico OU build monolítico da Mesa.
3. libwayland-client (ou EGL surfaceless + dumb).
4. libdrm portada (usa os ioctls acima como base).
5. EXECBUFFER+CTX no kernel + TGSI de teste.
