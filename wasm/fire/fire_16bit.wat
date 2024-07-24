;; FIRE_WIDTH = 128
;; FIRE_HEIGHT = 128
;; FIRE_WIDTH * FIRE_HEIGHT = 16384
;; FIRE_WIDTH * (FIRE_HEIGHT - 1) = 16256
(import "Math" "random" (func $random (result f32)))

;; 1 pages * 64KiB bytes per page:
;; [0, 16384)       => firePixels, 1 byte per pixel.
;; [16384, 49152)   => canvasData, 2 bytes per pixel
;; [49152, 49226) => Palette data, RGBA.
(memory (export "mem") 1)

;; Palette data.
(data (i32.const 49152)
  "\00\20\18\20\28\60\40\60\50\A0\60\E0\70\E0\89\20\99\60\A9\E0\BA\20\C2\20"
  "\DA\60\DA\A0\DA\A0\D2\E0\D2\E0\D3\21\CB\61\CB\A1\CB\E1\CC\22\C4\22\C4\62"
  "\C4\A3\BC\E3\BC\E3\BD\24\BD\24\BD\65\B5\65\B5\A5\B5\A6\CE\6D\DE\F3\EF\78"
  "\FF\FF")

(func $setup
  (local $i i32)

  ;; Fill bottom row with color 36, (R=0xff, G=0xff, B=0xff).
  (local.set $i (i32.const 128))
  (loop
    ;; memory[12672 - 1 + i] = 36
    (i32.store8 offset=16256 (local.get $i) (i32.const 36))
    ;; loop if --i != 0
    (br_if 0
      (local.tee $i (i32.sub (local.get $i) (i32.const 1))))))

;; Run setup at start.
(start $setup)

(func (export "run")
  (local $i i32)
  (local $pixel i32)
  (local $randIdx i32)

  ;; Update the fire.
  (loop $xloop
    (loop $yloop
      (if
        ;; if (pixel = memory[i += 128]) != 0
        (local.tee $pixel
          (i32.load8_u
            (local.tee $i
              (i32.add (local.get $i) (i32.const 128)))))
        (then
          ;; randIdx = round(random() * 3.0) & 3
          (local.set $randIdx
            (i32.and
              (i32.trunc_f32_u
                (f32.nearest
                  (f32.mul
                    (call $random)
                    (f32.const 3))))
              (i32.const 3)))

          ;; memory[i - randIdx - 127] = pixel - (randIdx & 1)
          (i32.store8
            (i32.sub
              (i32.sub
                (local.get $i)
                (local.get $randIdx))
              (i32.const 127))
            (i32.sub
              (local.get $pixel)
              (i32.and
                (local.get $randIdx)
                (i32.const 1)))))
        (else
          ;; memory[i - 128] = 0
          (i32.store8
            (i32.sub (local.get $i) (i32.const 128))
            (i32.const 0))))

      ;; loop if i < 16384 - 128
      (br_if $yloop
        (i32.lt_u (local.get $i) (i32.const 16256))))

    ;; i -= 16384 - 128 - 1, loop if i != 128
    (br_if $xloop
      (i32.ne
        (local.tee $i (i32.sub (local.get $i) (i32.const 16255)))
        (i32.const 128))))

  ;; copy from firePixels to canvasData, using palette data.
  (local.set $i (i32.const 16384))
  (loop
    ;; --i
    (local.set $i (i32.sub (local.get $i) (i32.const 1)))

    ;; memory[16384 + (i << 1)] = memory[49152 + (memory[i] << 1)]
    (i32.store16 offset=16384
      (i32.shl (local.get $i) (i32.const 1))
      (i32.load16_u offset=49152
        (i32.shl (i32.load8_u (local.get $i)) (i32.const 1))
      )
    )

    ;; loop if i != 0
    (br_if 0 (local.get $i))))
