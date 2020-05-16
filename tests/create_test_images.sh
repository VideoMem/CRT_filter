convert encoded.bmp loop.bmp
convert -flop loop.bmp loop_flop.bmp
convert -flip loop.bmp loop_flip.bmp
convert -flip -flop loop.bmp loop_flipflop.bmp
convert -sharpen 1 loop.bmp loop_blurry.bmp
convert loop.bmp loop_copied.bmp
convert -modulate 70 loop.bmp loop_modulate.bmp
convert -negate loop.bmp loop_negate.bmp
convert -channel RGB -colors 32 loop.bmp loop_noisy.bmp
#convert loop.jpg loop_noisy.jpg
convert loop.bmp loop.webp
convert loop.webp loop_webp.bmp
