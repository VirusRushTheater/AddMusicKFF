import os
import requests
import re
import io
import shutil
import zipfile
import logging
import argparse
import sys
from pathlib import Path

SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
log = logging.getLogger(SCRIPT_NAME)

class AMKTestToolkit:
    def __init__(self,
        boilerplate_path : Path,
        saveto_path : Path
    ):
        self.boilerplate = boilerplate_path
        self.saveto = saveto_path

        if self.saveto.exists():
            shutil.rmtree(self.saveto)
        shutil.copytree(self.boilerplate, self.saveto)

        with open(self.saveto / "Addmusic_list.txt", "r+") as fd:
            addmusic_lines = fd.readlines()
            self.current_cursor = max([int(w.group(1), base=16) for w in [re.match(r"([0-9a-fA-F]{2})\s+[\w\/\\\.\- ]+$", v) for v in addmusic_lines] if w is not None])
            fd.write("\n")
        log.debug(f"Copied the Addmusic boilerplate. Found {self.current_cursor} songs.")
        self.tohex = lambda u : hex(u)[2:].upper()

    def downloadSong(self,
        address : str = None
    ):
        if (address is None):
            log.debug("Retrieving a random song from SMWCentral...")
            randomreq = requests.get("https://www.smwcentral.net/?p=section&a=random&s=smwmusic&u=0")
            if (randomreq.status_code // 100) != 2:
                raise requests.exceptions.ConnectionError("Could not retrieve a random SMWCentral page.")
            songhref = re.search(r'<a class="button action" href="([a-zA-Z0-9\/\._\-\%]+)">Download<\/a>', randomreq.content.decode("utf8"))
            if not songhref:
                raise RuntimeError("No match for the download button where there should be one.")
            zipaddress = "https:" + songhref.group(1)
        else:
            zipaddress = address
        log.debug(f"Downloading song: {zipaddress}")
        zipreq = requests.get(zipaddress)
        if (zipreq.status_code // 100) != 2:
            raise requests.exceptions.ConnectionError("Could not download the SMWCentral zip file.")

        memcached_zipfile = io.BytesIO(zipreq.content)
        mmlzipfile = zipfile.ZipFile(memcached_zipfile)

        file_list = mmlzipfile.filelist
        l_directories = [v.filename.strip("/") for v in file_list if v.is_dir() == True]
        l_files = [v.filename.strip("/") for v in file_list if v.is_dir() == False]

        # Retrieves the basedir (if the entire content of the archive is within one folder or not)
        basedir = [""]
        for l_d in l_directories:
            if all([l_f.startswith(l_d + "/") for l_f in l_files]):
                basedir.append(l_d)
        basedir = Path(max(basedir, key=len))

        # File list with its basedir part trimmed.
        l_files = [str(Path(l_f).relative_to(basedir)) for l_f in l_files]

        # Autodetect the music .txt file and opens it.
        music_txt_filename = str(basedir / min(filter(lambda u: u.endswith(".txt"), l_files), key=lambda v: len(v) + 100*v.count("/")))
        with mmlzipfile.open(music_txt_filename) as fd:
            music_txt_content = fd.read()
            # Conflicting encoding types in special characters like the é in "Pokémon" makes me want to kill a puppy.
            try:
                music_txt_content = music_txt_content.decode("utf8")
            except UnicodeDecodeError:
                music_txt_content = music_txt_content.decode("iso8859")
        
        # Special directives like these tell whether this is, in fact, a MML text file.
        if not any([(v in music_txt_content) for v in ["#am", "#SPC", "#0", "#1", "#2", "#3", "#4", "#5", "#6", "#7"]]):
            log.debug(f"The txt file found does not represent a normal song. Did not extract it.")
            # Not a MML file.
            return None

        # TODO: Musics with Addmusic_sample groups.txt files. I have not faced them... yet.
        # -> https://dl.smwcentral.net/31941/Rusty_MJ.zip
        # samples without path -> https://dl.smwcentral.net/8618/Battle01.zip
        
        log.debug(f"Extracted txt: {music_txt_filename}")

        # Find any Sample Groups text file that might need to be copied to the repo's main
        sample_group_clues = [l_f for l_f in l_files if all(map(lambda v: v in l_f, ["sample", "group"]))]
        if len(sample_group_clues) > 0:
            spgroupfile = str(basedir / sample_group_clues[0])
            with mmlzipfile.open(spgroupfile) as fd:
                spgrouptext = fd.read().decode("utf8")
            
            with open(self.saveto / "Addmusic_sample groups.txt", "a") as fd:
                fd.write("\n\n")
                fd.write(spgrouptext)
            log.debug(f"Updated Addmusic_sample groups.txt")

        # Extract the .brr files in the proper directory.
        if ("#samples" in music_txt_content):
            # The #path directive tells where is the base folder where to find the .brr samples.
            if ("#path" in music_txt_content):
                rpath = re.search(r"#path\s+\"([\w\-. \#\$@]+)\"", music_txt_content).group(1)
            else:
                rpath = ""
            
            brr_folder = str(basedir / rpath)
            brr_folder += "/" if (len(brr_folder) > 0) else ""
            brrsamples = [l_f for l_f in l_files if l_f.startswith(brr_folder) and l_f.endswith(".brr")]
            for brrsp_i in brrsamples:
                mmlzipfile.extract(brrsp_i, path=self.saveto / "samples")
            log.debug(f"Extracted samples from: {music_txt_filename}")

        # The MML is written like this in case it's necessary to fix the encoding.
        with open(self.saveto / "music" / music_txt_filename, "w") as fd:
            fd.write(music_txt_content)
        
        # Append the MML to the Addmusic_list text file.
        self.current_cursor += 1
        with open(self.saveto / "Addmusic_list.txt", "a") as fd:
            fd.write(f"{self.tohex(self.current_cursor)}  {music_txt_filename}\n")

if __name__ == "__main__":
    log.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stderr)
    handler.setLevel(logging.DEBUG)
    # formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    formatter = logging.Formatter('-- %(name)s - %(message)s')
    handler.setFormatter(formatter)
    log.addHandler(handler)

    argp = argparse.ArgumentParser(description="Tool for making test environments for AddmusicK")
    argp.add_argument("--boilerplate", help="AMK Boilerplate folder")
    argp.add_argument("--saveto", help="AMK environment output directory")
    argp.add_argument("-n", type=int, help="Amount of random songs to be downloaded")
    args = argp.parse_args()

    tk = AMKTestToolkit(Path(args.boilerplate), Path(args.saveto))
    for _ in range(args.n):
        tk.downloadSong()
