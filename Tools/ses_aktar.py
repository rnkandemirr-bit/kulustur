# Unreal icinde calisir: Tools/Sesler/*.wav dosyalarini /Game/Kulustur/Audio altina aktarir.
# UnrealEditor-Cmd.exe kulustur.uproject -run=pythonscript -script="<bu dosya>"
import os
import unreal

ROOT = os.path.join(unreal.Paths.project_dir(), "Tools", "Sesler")
DEST = "/Game/Kulustur/Audio"
LOOPS = {"Ruzgar", "Yagmur", "Sehir", "Ugultu", "Lazer"}

tasks = []
for f in sorted(os.listdir(ROOT)):
    if not f.lower().endswith(".wav"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, f)
    t.destination_path = DEST
    t.destination_name = os.path.splitext(f)[0]
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for t in tasks:
    name = t.destination_name
    path = DEST + "/" + name
    a = unreal.load_asset(path)
    if a is None:
        unreal.log_error("KULUSTUR SES YOK: " + path)
        continue
    if name in LOOPS:
        a.set_editor_property("looping", True)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log("KULUSTUR SES AKTARILDI: " + path)
