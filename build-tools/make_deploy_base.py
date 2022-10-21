import tarfile
import tempfile
import os
import shutil
import argparse

if __name__ == "__main__":
	pass

	def make_tarfile(output_filename, source_dir):
		with tarfile.open(output_filename, "w:gz") as tar:
			tar.add(source_dir, arcname=os.path.basename(source_dir))