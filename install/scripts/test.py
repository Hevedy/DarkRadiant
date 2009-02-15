# Some interface tests

# Test the Registry interface
value = GlobalRegistry.get('user/paths/appPath')
print(value)

worldspawn = Radiant.findEntityByClassname("worldspawn")
worldspawn.setKeyValue('test', 'success')
print('Worldspawn edited')

# Test the EClassManager interface
eclass = GlobalEntityClassManager.findClass('atdm:func_shooter')
print(eclass.getAttribute('editor_usage').value)

modelDef = GlobalEntityClassManager.findModel('builderforger')
print('ModelDef mesh for builderforger = ' + modelDef.mesh)

# Test iterating over C++ std::map
#for anim in modelDef.anims:
#	print(anim.key())
#	print(' = ')
#	print(anim.data())
#	print('')

# Test implementing a eclass visitor interface
#class TestVisitor(EntityClassVisitor) :
#	def visit(self, eclass):
#		print eclass.getAttribute('editor_usage').value

#eclassVisitor = TestVisitor()
#GlobalEntityClassManager.forEach(eclassVisitor)

# Test traversing the scenegraph
class SceneWalker(SceneNodeVisitor) :
	def pre(self, node):
		print(node.getNodeType())
		return 1

walker = SceneWalker()
GlobalSceneGraph.root().traverse(walker)

print('')
