# -*- mode: python -*-

block_cipher = None


a = Analysis(['D:\\Development\\Retina\\Gears\\GearsPy\\Gears.pyw'],
             pathex=['D:\\Development\\Retina\\Gears\\GearsPyInstallerPack'],
             binaries=None,
             datas=None,
             hiddenimports=['six', 'packaging', 'packaging.version', 'packaging.specifiers'],
             hookspath=[],
             runtime_hooks=[],
             excludes=['Project'],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          exclude_binaries=True,
          name='Gears',
          debug=False,
          strip=False,
          upx=True,
          console=True )
coll = COLLECT(exe,
               a.binaries,
               a.zipfiles,
               a.datas,
               strip=False,
               upx=True,
               name='Gears')
