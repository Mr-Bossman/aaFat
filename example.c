#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include <assert.h>
#include "aaFat.h"

int main(){
    write_FAT();
    printf("%d\n",FAT_ERRpop());
    new_file("br");
    printf("%d\n",FAT_ERRpop());
    print_file_table();

    new_file("3");
    printf("%d\n",FAT_ERRpop());
    print_fat();
    printf("%d\n",get_file_block("3"));
    printf("%d\n",FAT_ERRpop());
    print_file_table();
    del_file("br");
    printf("%d\n",FAT_ERRpop());
    print_file_table();
    print_fat();
    char data[] ="Bee Movie Script - Dialogue TranscriptVoila! Finally, the Bee Movie script is here for all you fans of the Jerry Seinfeld animated movie. This puppy is a transcript that was painstakingly transcribed using the screenplay and/or viewings of the movie to get the dialogue. I know, I know, I still need to get the cast names in there and all that jazz, so if you have any corrections, feel free to drop me a line. At least you'll have some Bee Movie quotes (or even a monologue or two) to annoy your coworkers with in the meantime, right?And swing on back to Drew's Script-O-Rama afterwards -- because reading is good for your noodle. Better than Farmville, anyway.Bee Movie Script    According to all known lawsof aviation,  there is no way a beeshould be able to fly.  Its wings are too small to getits fat little body off the ground.  The bee, of course, flies anyway  because bees don't carewhat humans think is impossible.  Yellow, black. Yellow, black.Yellow, black. Yellow, black.  Ooh, black and yellow!Let's shake it up a little.  Barry! Breakfast is ready!  Ooming!  Hang on a second.  Hello?  - Barry?- Adam?  - Oan you believe this is happening?- I can't. I'll pick you up.  Looking sharp.  Use the stairs. Your fatherpaid good money for those.  Sorry. I'm excited.  Here's the graduate.We're very proud of you, son.  A perfect report card, all B's.  Very proud.  Ma! I got a thing going here.  - You got lint on your fuzz.- Ow! That's me!  - Wave to us! We'll be in row 118,000.- Bye!  Barry, I told you,stop flying in the house!  - Hey, Adam.- Hey, Barry.  - Is that fuzz gel?- A little. Special day, graduation.  Never thought I'd make it.  Three days grade school,three days high school.  Those were awkward.  Three days college. I'm glad I tooka day and hitchhiked around the hive.  You did come back different.  - Hi, Barry.- Artie, growing a mustache? Looks good.  - Hear about Frankie?- Yeah.  - You going to the funeral?- No, I'm not going.  Everybody knows,sting someone, you die.  Don't waste it on a squirrel.Such a hothead.  I guess he could havejust gotten out of the way.  I love this incorporatingan amusement park into our day.  That's why we don't need vacations.  Boy, quite a bit of pomp...under the circumstances.  - Well, Adam, today we are men.- We are!  - Bee-men.- Amen!  Hallelujah!  Students, faculty, distinguished bees,  please welcome Dean Buzzwell.  Welcome, New Hive Oitygraduating class of...  ...9:15.  That concludes our ceremonies.  And begins your careerat Honex Industries!  Will we pick ourjob today?  I heard it's just orientation.  Heads up! Here we go.  Keep your hands and antennasinside the tram at all times.  - Wonder what it'll be like?- A little scary.  Welcome to Honex,a division of Honesco  and a part of the Hexagon Group.  This is it!  Wow.  Wow.  We know that you, as a bee,have worked your whole life  to get to the point where youcan work for your whole life.  Honey begins when our valiant PollenJocks bring the nectar to the hive.  Our top-secret formula  is automatically color-corrected,scent-adjusted and bubble-contoured  into this soothing sweet syrup  with its distinctivegolden glow you know as...  Honey!  - That girl was hot.- She's my cousin!  - She is?- Yes, we're all cousins.  - Right. You're right.- At Honex, we constantly strive  to improve every aspectof bee existence.  These bees are stress-testinga new helmet technology.  - What do you think he makes?- Not enough.  Here we have our latest advancement,the Krelman.  - What does that do?- Oatches that little strand of honey  that hangs after you pour it.Saves us millions.  Oan anyone work on the Krelman?  Of course. Most bee jobs aresmall ones. But bees know  that every small job,if it's ";
    write_file("3",data,sizeof(data));
    printf("%d\n",FAT_ERRpop());
    memset(data,0,sizeof(data));
    read_file("3",data,sizeof(data));
    printf("%d\n",FAT_ERRpop());
    puts(data);
    print_file_table();
    print_fat();
    return 0;
}