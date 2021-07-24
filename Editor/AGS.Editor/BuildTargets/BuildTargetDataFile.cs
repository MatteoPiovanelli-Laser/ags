using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class BuildTargetDataFile : BuildTargetBase
    {
        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            return new Dictionary<string, string>();
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[] { GetCompiledPath() };
        }

        public override void DeleteMainGameData(string name)
        {
            string filename = Path.Combine(OutputDirectoryFullPath, name + ".ags");
            Utilities.DeleteFileIfExists(filename);
        }

        private void DeleteAnyExistingSplitResourceFiles()
        {
            foreach (string fileName in Utilities.GetDirectoryFileList(GetCompiledPath(), Factory.AGSEditor.BaseGameFileName + ".0*"))
            {
                File.Delete(fileName);
            }
        }

        /// <summary>
        /// Creates a list of game resources as a list of tuples:
        /// - first tuple's element is resource's name,
        /// - second is real file path
        /// </summary>
        private Tuple<string, string>[] ConstructFileListForDataFile(CompileMessages errors)
        {
            List<string> files = new List<string>();
            Environment.CurrentDirectory = Factory.AGSEditor.CurrentGame.DirectoryPath;
            Utilities.AddAllMatchingFiles(files, "preload.pcx");
            Utilities.AddAllMatchingFiles(files, AGSEditor.SPRITE_INDEX_FILE_NAME);
            foreach (AudioClip clip in Factory.AGSEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.BundlingType == AudioFileBundlingType.InGameEXE)
                {
                    files.Add(clip.CacheFileName);
                }
            }
            Utilities.AddAllMatchingFiles(files, "flic*.fl?");
            Utilities.AddAllMatchingFiles(files, AGSEditor.COMPILED_DTA_FILE_NAME);
            Utilities.AddAllMatchingFiles(files, "agsfnt*.ttf");
            Utilities.AddAllMatchingFiles(files, "agsfnt*.wfn");
            Utilities.AddAllMatchingFiles(files, AGSEditor.SPRITE_FILE_NAME);
            foreach (UnloadedRoom room in Factory.AGSEditor.CurrentGame.RootRoomFolder.AllItemsFlat)
            {
                if (File.Exists(room.FileName))
                {
                    files.Add(room.FileName);
                }
            }
            Utilities.AddAllMatchingFiles(files, "*.ogv");

            // Regular files are registered under their filenames (w/o dir)
            var assetList = new List<Tuple<string, string>>();
            foreach (var f in files)
            {
                assetList.Add(new Tuple<string, string>(Path.GetFileName(f), f));
            }

            var userFiles = ConstructCustomFileListForDataFile(errors);
            if (userFiles != null) assetList.AddRange(userFiles);
            return assetList.ToArray();
        }

        /// <summary>
        /// Creates a list of custom game resources as a list of tuples:
        /// - first tuple's element is resource's name,
        /// - second is real file path
        /// </summary>
        private Tuple<string, string>[] ConstructCustomFileListForDataFile(CompileMessages errors)
        {
            string userOpt = Factory.AGSEditor.CurrentGame.Settings.CustomDataDir;
            if (string.IsNullOrEmpty(userOpt)) return null;

            string curDir = Factory.AGSEditor.CurrentGame.DirectoryPath;
            string[] restrictedDirs = new string[] {
                Path.Combine(curDir, AGSEditor.OUTPUT_DIRECTORY),
                Path.Combine(curDir, AGSEditor.DEBUG_OUTPUT_DIRECTORY),
                Path.Combine(curDir, "AudioCache"),
                Path.Combine(curDir, "Speech") };

            List<string> userFiles = new List<string>();
            string[] user_dirs = userOpt.Split(',');
            Array.Sort(user_dirs);
            List<string> usedCustomDirs = new List<string>();

            foreach (var dir in user_dirs)
            {
                // don't allow absolute paths
                if (Path.IsPathRooted(dir))
                {
                    errors.Add(new CompileWarning(
                        string.Format("Cannot use absolute path as a custom data source: {0}", dir)));
                    continue;
                }
                // don't allow paths with "." or ".." in them
                if (Utilities.DoesPathContainDotDirs(dir))
                {
                    errors.Add(new CompileWarning(
                        string.Format("Cannot use path containing '.' or '..' as a custom data source: {0}", dir)));
                    continue;
                }
                // don't allow paths that match or inside restricted folders,
                // or folders that were already used to make a list of assets
                string testDir = Path.Combine(curDir, dir);
                if (Utilities.PathIsSameOrNestedAmong(testDir, restrictedDirs))
                {
                    errors.Add(new CompileWarning(
                        string.Format("Cannot use restricted location as a custom data source: {0}", dir)));
                    continue;
                }
                if (Utilities.PathIsSameOrNestedAmong(testDir, usedCustomDirs.ToArray()))
                    continue; // don't report, simply skip

                Utilities.AddAllMatchingFiles(userFiles, dir, "*", true, SearchOption.AllDirectories);
                usedCustomDirs.Add(Path.Combine(curDir, dir));
            }

            // User files are registered under their relative paths
            var assetList = new List<Tuple<string, string>>();
            foreach (var f in userFiles)
            {
                assetList.Add(new Tuple<string, string>(f, f));
            }
            return assetList.ToArray();
        }

        private void CreateAudioVOXFile(bool forceRebuild)
        {
            List<string> fileListForVox = new List<string>();
            string audioVox = GetCompiledPath(AGSEditor.AUDIO_VOX_FILE_NAME);
            bool rebuildVox = (!File.Exists(audioVox)) || (forceRebuild);

            foreach (AudioClip clip in Factory.AGSEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.BundlingType == AudioFileBundlingType.InSeparateVOX)
                {
                    string thisFileName = clip.CacheFileName;
                    if (File.GetLastWriteTimeUtc(thisFileName) != clip.FileLastModifiedDate)
                    {
                        rebuildVox = true;
                        clip.FileLastModifiedDate = File.GetLastWriteTimeUtc(thisFileName);
                    }
                    fileListForVox.Add(thisFileName);
                }
            }

            if (File.Exists(audioVox) &&
                (fileListForVox.Count == 0) || (rebuildVox))
            {
                File.Delete(audioVox);
            }

            if ((rebuildVox) && (fileListForVox.Count > 0))
            {
                Factory.NativeProxy.CreateVOXFile(audioVox, fileListForVox.ToArray());
            }
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            Factory.AGSEditor.SetMODMusicFlag();
            DeleteAnyExistingSplitResourceFiles();
            if (!DataFileWriter.SaveThisGameToFile(AGSEditor.COMPILED_DTA_FILE_NAME, Factory.AGSEditor.CurrentGame, errors))
            {
                return false;
            }
            string errorMsg = DataFileWriter.MakeDataFile(ConstructFileListForDataFile(errors),
                Factory.AGSEditor.CurrentGame.Settings.SplitResources * 1000000,
                Factory.AGSEditor.BaseGameFileName, true);
            if (errorMsg != null)
            {
                errors.Add(new CompileError(errorMsg));
            }
            File.Delete(AGSEditor.COMPILED_DTA_FILE_NAME);
            CreateAudioVOXFile(forceRebuild);
            // Update config file with current game parameters
            Factory.AGSEditor.WriteConfigFile(GetCompiledPath());
            return true;
        }

        public override bool IsTargetedForBuild
        {
            get
            {
                return true;
            }
        }

        public override string Name
        {
            get
            {
                return BuildTargetsInfo.DATAFILE_TARGET_NAME;
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return AGSEditor.DATA_OUTPUT_DIRECTORY;
            }
        }
    }
}
