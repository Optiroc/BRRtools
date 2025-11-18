import argparse
import subprocess
import os
import sys
from pathlib import Path
from time import sleep

from tqdm import tqdm


class BatchProcessor:

    def __init__(self, brrtools_dir, indir, outdir, rate):
        self.infiles = BatchProcessor.get_infiles(indir)
        self.outpath = BatchProcessor.get_outpath(outdir)
        self.rate = rate
        self.brrtools_bin = BatchProcessor.get_brrtools_bin(brrtools_dir)

        # Find Cygwin bash on Windows
        if sys.platform == "win32":
            self.cygwin_bash = self._find_cygwin_bash()
            print(f"Using Cygwin bash: {self.cygwin_bash}")

    def process_batch(self):
        success_count = 0
        error_count = 0

        for infile in tqdm(self.infiles, desc="Processing"):
            try:
                self.process_file(infile)
                success_count += 1
            except subprocess.CalledProcessError as e:
                error_count += 1
                print(f"\n{'='*70}", file=sys.stderr)
                print(f"ERROR processing {infile.name}", file=sys.stderr)
                print(f"Exit code: {e.returncode}", file=sys.stderr)
                if e.stdout:
                    print(f"Stdout: {e.stdout}", file=sys.stderr)
                if e.stderr:
                    print(f"Stderr: {e.stderr}", file=sys.stderr)
                print(f"{'='*70}", file=sys.stderr)
                continue
            except Exception as e:
                error_count += 1
                print(f"\nUnexpected error processing {infile.name}: {e}", file=sys.stderr)
                import traceback
                traceback.print_exc()
                continue

        print(f"\n{'='*70}")
        print(f"Batch processing complete!")
        print(f"  Success: {success_count}/{len(self.infiles)}")
        print(f"  Errors:  {error_count}/{len(self.infiles)}")
        print(f"{'='*70}")

    @staticmethod
    def get_infiles(indir):
        dirpath = Path(indir)
        if not dirpath.is_dir():
            raise ValueError(f"Input directory does not exist: {dirpath}")

        infiles = [item for item in dirpath.iterdir() 
                   if item.is_file() and item.suffix.lower() == ".wav"]

        if not infiles:
            raise ValueError(f"No .wav files found in {dirpath}")

        print(f"Found {len(infiles)} WAV files to process")
        return infiles

    @staticmethod
    def get_outpath(outdir):
        outpath = Path(outdir)
        if not outpath.exists():
            os.makedirs(outpath, exist_ok=True)
            print(f"Created output directory: {outpath}")

        if not outpath.is_dir():
            raise ValueError(f"Output path exists but is not a directory: {outpath}")

        return outpath

    @staticmethod
    def get_brrtools_bin(brrtools_dir):
        brrtools_path = Path(brrtools_dir)
        if not brrtools_path.is_dir():
            raise ValueError(f"BRRtools directory does not exist: {brrtools_path}")

        brrtools_bin = brrtools_path / "bin"
        if not brrtools_bin.is_dir():
            raise ValueError(f"BRRtools bin directory does not exist: {brrtools_bin}")

        return brrtools_bin

    @staticmethod
    def _find_cygwin_bash():
        """Find Cygwin bash.exe, avoiding WSL bash"""
        # Common Cygwin installation paths
        possible_paths = [
            Path("C:/cygwin64/bin/bash.exe"),
            Path("C:/cygwin/bin/bash.exe"),
        ]

        # Check environment variable
        cygwin_root = os.environ.get("CYGWIN_ROOT")
        if cygwin_root:
            possible_paths.insert(0, Path(cygwin_root) / "bin" / "bash.exe")

        # Check standard locations first
        for bash_path in possible_paths:
            if bash_path.exists():
                return str(bash_path)

        # Try to find all bash.exe locations and filter for Cygwin
        try:
            result = subprocess.run(
                ["where", "bash.exe"],
                capture_output=True,
                text=True,
                check=False
            )
            if result.returncode == 0:
                bash_locations = result.stdout.strip().split('\n')
                for location in bash_locations:
                    location = location.strip()
                    # Skip WSL bash (in System32 or system32)
                    if "system32" in location.lower():
                        continue
                    # Accept Cygwin bash
                    if "cygwin" in location.lower() and Path(location).exists():
                        return location
        except Exception:
            pass

        raise FileNotFoundError(
            "Could not find Cygwin bash.exe.\n"
            "Make sure Cygwin is installed.\n"
            "If Cygwin is installed in a non-standard location, set CYGWIN_ROOT environment variable.\n"
            "Example: set CYGWIN_ROOT=C:\\cygwin64"
        )

    def process_file(self, infile):
        """Process a single WAV file through BRR encoding and decoding.

        Handles both Unix/Linux and Windows (Cygwin) environments.
        """
        # Prepare file paths
        in_wav = str(infile)
        tmp_brr = self.outpath / f"{infile.stem}.brr"
        out_wav = self.outpath / infile.name

        # Determine executable names based on platform
        if sys.platform == "win32":
            # Windows with Cygwin executables
            brr_encoder_exe = "brr_encoder"
            brr_decoder_exe = "brr_decoder"

            # Convert Windows paths to Cygwin format
            in_wav_path = self._windows_to_cygwin_path(in_wav)
            tmp_brr_path = self._windows_to_cygwin_path(str(tmp_brr))
            out_wav_path = self._windows_to_cygwin_path(str(out_wav))

            # Full path to Cygwin executables
            brr_encoder = self._windows_to_cygwin_path(str(self.brrtools_bin / brr_encoder_exe))
            brr_decoder = self._windows_to_cygwin_path(str(self.brrtools_bin / brr_decoder_exe))

            # Run through Cygwin bash (NOT WSL bash!)
            encode_cmd = f'"{brr_encoder}" -sb{self.rate} "{in_wav_path}" "{tmp_brr_path}"'
            decode_cmd = f'"{brr_decoder}" -s{self.rate} -g "{tmp_brr_path}" "{out_wav_path}"'

            subprocess.run(
                [self.cygwin_bash, "-c", encode_cmd],
                check=True,
                capture_output=True,
                text=True
            )
            subprocess.run(
                [self.cygwin_bash, "-c", decode_cmd],
                check=True,
                capture_output=True,
                text=True
            )
        else:
            # Mac/Linux - native executables
            brr_encoder = str(self.brrtools_bin / "brr_encoder")
            brr_decoder = str(self.brrtools_bin / "brr_decoder")

            subprocess.run(
                [brr_encoder, f"-sb{self.rate}", in_wav, str(tmp_brr)],
                check=True,
                capture_output=True,
                text=True
            )
            subprocess.run(
                [brr_decoder, f"-s{self.rate}", "-g", str(tmp_brr), str(out_wav)],
                check=True,
                capture_output=True,
                text=True
            )

        # Delete temporary BRR file
        if tmp_brr.exists():
            retries = 0
            # Add a retry loop with a short wait in case the file is temporarily locked by the OS.
            while retries < 6:
                try:
                    retries += 1
                    os.remove(tmp_brr)
                    break
                except Exception as e_del:
                    if retries < 6:
                        sleep(0.05)
                        continue
                    print(f"Warning: Could not remove temporary BRR file: {e_del}", file=sys.stderr)
                    break

    @staticmethod
    def _windows_to_cygwin_path(win_path):
        """Convert Windows path to Cygwin /cygdrive format.

        Examples:
            C:\\Users\\file.wav -> /cygdrive/c/Users/file.wav
            D:\\Music\\song.wav -> /cygdrive/d/Music/song.wav
        """
        path = Path(win_path).resolve()
        parts = path.parts

        # Check if path has a drive letter (e.g., C:\\)
        if len(parts) > 0 and len(parts[0]) == 3 and parts[0][1] == ':':
            drive = parts[0][0].lower()
            rest = '/'.join(parts[1:])
            return f"/cygdrive/{drive}/{rest}"
        else:
            # Relative or UNC path - just convert backslashes
            return str(path).replace('\\', '/')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Batch process WAV files to make them sound like SNES sounds",
        epilog="Requires Cygwin on Windows. Set CYGWIN_ROOT if installed in non-standard location."
    )
    parser.add_argument("brrtools", type=str, help="Path to BRRTools directory")
    parser.add_argument("indir", type=str, help="Path to input directory with WAV files")
    parser.add_argument("outdir", type=str, help="Path to output directory")
    parser.add_argument(
        "--rate", "-r", type=int, default=16000,
        help="Sample rate in Hz for BRR compression (default: 16000)"
    )

    args = parser.parse_args()

    try:
        processor = BatchProcessor(args.brrtools, args.indir, args.outdir, args.rate)
        processor.process_batch()
        print("\nDONE")
    except Exception as e:
        print(f"\nFATAL ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
