//
// Created by sebastian on 23/5/20.
//

#include <vips/vips8>

using namespace vips;

int
main (int argc, char **argv)
{
    if (VIPS_INIT (argv[0]))
        vips_error_exit (NULL);

    if (argc != 3)
        vips_error_exit ("usage: %s input-file output-file", argv[0]);

    VImage in = VImage::new_from_file (argv[1],
                                       VImage::option ()->set ("access", VIPS_ACCESS_SEQUENTIAL));

    double avg = in.avg ();

    printf ("avg = %g\n", avg);
    printf ("width = %d\n", in.width ());

    in = VImage::new_from_file (argv[1],
                                VImage::option ()->set ("access", VIPS_ACCESS_SEQUENTIAL));

    VImage out = in.embed (10, 10, 1000, 1000,
                           VImage::option ()->
                                   set ("extend", "background")->
                                   set ("background", 128));

    out.write_to_file (argv[2]);

    vips_shutdown ();

    return (0);
}