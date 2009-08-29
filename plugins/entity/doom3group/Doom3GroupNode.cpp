#include "Doom3GroupNode.h"

#include "../curve/CurveControlPointFunctors.h"

namespace entity {

Doom3GroupNode::Doom3GroupNode(const IEntityClassConstPtr& eclass) :
	EntityNode(eclass),
	m_contained(
		*this, // Pass <this> as Doom3GroupNode&
		Node::TransformChangedCaller(*this),
		Node::BoundsChangedCaller(*this)
	),
	m_curveNURBS(m_contained.m_curveNURBS,
				 SelectionChangedComponentCaller(*this)),
	m_curveCatmullRom(m_contained.m_curveCatmullRom, 
					  SelectionChangedComponentCaller(*this)),
	_originInstance(VertexInstance(m_contained.getOrigin(), SelectionChangedComponentCaller(*this))),
	_updateSkin(true),
	_instantiated(false)
{
	construct();
}

Doom3GroupNode::Doom3GroupNode(const Doom3GroupNode& other) :
	EntityNode(other),
	scene::Cloneable(other),
	scene::GroupNode(other),
	Snappable(other),
	TransformNode(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	Bounded(other),
	CurveNode(other),
	m_contained(
		other.m_contained,
		*this, // Pass <this> as Doom3GroupNode&
		Node::TransformChangedCaller(*this),
		Node::BoundsChangedCaller(*this)
	),
	m_curveNURBS(m_contained.m_curveNURBS,
				 SelectionChangedComponentCaller(*this)),
	m_curveCatmullRom(m_contained.m_curveCatmullRom, 
					  SelectionChangedComponentCaller(*this)),
	_originInstance(VertexInstance(m_contained.getOrigin(), SelectionChangedComponentCaller(*this))),
	_updateSkin(true),
	_instantiated(false)
{
	// greebo: Don't call construct() here, this should be invoked by the
	// clone() method
}

Doom3GroupNode::~Doom3GroupNode() {
	m_contained.m_curveCatmullRom.disconnect(m_contained.m_curveCatmullRomChanged);
	m_contained.m_curveNURBS.disconnect(m_contained.m_curveNURBSChanged);

	removeKeyObserver("skin", SkinChangedCaller(*this));

	Callback emptyCallback;
	m_contained.setTransformChanged(emptyCallback);
	Node::detachTraverseObserver(this);
}

void Doom3GroupNode::construct()
{
	m_contained.construct();

	Node::attachTraverseObserver(this);

	// Attach the callback as keyobserver for the skin key
	addKeyObserver("skin", SkinChangedCaller(*this));

	m_contained.m_curveNURBSChanged = m_contained.m_curveNURBS.connect(
		CurveEditInstance::CurveChangedCaller(m_curveNURBS)
	);
	m_contained.m_curveCatmullRomChanged = m_contained.m_curveCatmullRom.connect(
		CurveEditInstance::CurveChangedCaller(m_curveCatmullRom)
	);
}

bool Doom3GroupNode::hasEmptyCurve() {
	return m_contained.m_curveNURBS.isEmpty() && 
		   m_contained.m_curveCatmullRom.isEmpty();
}

void Doom3GroupNode::appendControlPoints(unsigned int numPoints) {
	m_contained.appendControlPoints(numPoints);
}

void Doom3GroupNode::removeSelectedControlPoints() {
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.removeSelectedControlPoints();
		m_curveCatmullRom.write(curve_CatmullRomSpline, _entity);
	}
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.removeSelectedControlPoints();
		m_curveNURBS.write(curve_Nurbs, _entity);
	}
}

void Doom3GroupNode::insertControlPointsAtSelected() {
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.insertControlPointsAtSelected();
		m_curveCatmullRom.write(curve_CatmullRomSpline, _entity);
	}
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.insertControlPointsAtSelected();
		m_curveNURBS.write(curve_Nurbs, _entity);
	}
}

void Doom3GroupNode::convertCurveType() {
	m_contained.convertCurveType();
}

const AABB& Doom3GroupNode::localAABB() const {
	return m_contained.localAABB();
}

void Doom3GroupNode::addOriginToChildren() {
	if (!m_contained.isModel()) {
		Doom3BrushTranslator translator(m_contained.getOrigin());
		traverse(translator);
	}
}

void Doom3GroupNode::removeOriginFromChildren() {
	if (!m_contained.isModel()) {
		Doom3BrushTranslator translator(-m_contained.getOrigin());
		traverse(translator);
	}
}

void Doom3GroupNode::selectionChangedComponent(const Selectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool Doom3GroupNode::isSelectedComponents() const {
	return m_curveNURBS.isSelected() || m_curveCatmullRom.isSelected() || (m_contained.isModel() && _originInstance.isSelected());
}

void Doom3GroupNode::setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		m_curveNURBS.setSelected(selected);
		m_curveCatmullRom.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void Doom3GroupNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	if (mode == SelectionSystem::eVertex) {
		test.BeginMesh(localToWorld());
		m_curveNURBS.testSelect(selector, test);
		m_curveCatmullRom.testSelect(selector, test);
		
		_originInstance.testSelect(selector, test);
	}
}

void Doom3GroupNode::onRemoveFromScene() {
	// Call the base class first
	SelectableNode::onRemoveFromScene();

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);
}

const AABB& Doom3GroupNode::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();
	
	ControlPointBoundsAdder boundsAdder(m_aabb_component);
	m_curveNURBS.forEachSelected(boundsAdder);
	m_curveCatmullRom.forEachSelected(boundsAdder);
	
	if (_originInstance.isSelected()) {
		m_aabb_component.includePoint(_originInstance.getVertex());
	}
	return m_aabb_component;
}

void Doom3GroupNode::snapComponents(float snap) {
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.snapto(snap);
		m_curveNURBS.write(curve_Nurbs, _entity);
	}
	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.snapto(snap);
		m_curveCatmullRom.write(curve_CatmullRomSpline, _entity);
	}
	if (_originInstance.isSelected()) {
		m_contained.snapOrigin(snap);
	}
}

scene::INodePtr Doom3GroupNode::clone() const {
	boost::shared_ptr<Doom3GroupNode> clone(new Doom3GroupNode(*this));
	clone->setSelf(clone);
	clone->construct();
	return clone;
}

void Doom3GroupNode::instantiate(const scene::Path& path)
{
	_instantiated = true;

	Node::getTraversable().instanceAttach(path_find_mapfile(path.begin(), path.end()));
	EntityNode::instantiate(path);
}

void Doom3GroupNode::uninstantiate(const scene::Path& path)
{
	_instantiated = false;

	Node::getTraversable().instanceDetach(path_find_mapfile(path.begin(), path.end()));
	EntityNode::uninstantiate(path);
}

// Snappable implementation
void Doom3GroupNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& Doom3GroupNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

Entity& Doom3GroupNode::getEntity() {
	return _entity;
}

void Doom3GroupNode::testSelect(Selector& selector, SelectionTest& test) {
	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the Doom3Group class
	m_contained.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.valid()) {
		Selector_add(selector, getSelectable(), best);
	}
}

void Doom3GroupNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	// greebo: Check if the skin needs updating before rendering.
	if (_updateSkin) {
		if (m_contained.isModel()) {
			// Instantiate a walker class equipped with the new value
			SkinChangedWalker walker(_entity.getKeyValue("skin"));
			// Update all children
			traverse(walker);
		}

		_updateSkin = false;
	}

	m_contained.renderSolid(collector, volume, localToWorld(), isSelected());

	m_curveNURBS.renderComponentsSelected(collector, volume, localToWorld());
	m_curveCatmullRom.renderComponentsSelected(collector, volume, localToWorld());
}

void Doom3GroupNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld(), isSelected());

	m_curveNURBS.renderComponentsSelected(collector, volume, localToWorld());
	m_curveCatmullRom.renderComponentsSelected(collector, volume, localToWorld());
}

void Doom3GroupNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const {
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		m_curveNURBS.renderComponents(collector, volume, localToWorld());
		m_curveCatmullRom.renderComponents(collector, volume, localToWorld());
		
		// Register the renderable with OpenGL
		if (!m_contained.isModel()) {
			_originInstance.render(collector, volume, localToWorld());
		}
	}
}

void Doom3GroupNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.translate(
			getTranslation(), 
			getRotation() != c_quaternion_identity // FALSE for identity rotations 
		);
		m_contained.rotate(getRotation());

		// Transform all curves also in primitive mode
		// pass FALSE to force the transformation of non-selected points
		Matrix4 transformation = calculateTransform();
		m_curveNURBS.transform(transformation, false);
		m_curveCatmullRom.transform(transformation, false);
	}
	else {
		// Transform the components only
		transformComponents(calculateTransform());
	}
	// Trigger a recalculation of the curve's controlpoints
	m_contained.m_curveNURBS.curveChanged();
	m_contained.m_curveCatmullRom.curveChanged();
}

void Doom3GroupNode::transformComponents(const Matrix4& matrix) {
	if (m_curveNURBS.isSelected()) {
		m_curveNURBS.transform(matrix);
	}

	if (m_curveCatmullRom.isSelected()) {
		m_curveCatmullRom.transform(matrix);
	}

	if (_originInstance.isSelected()) {
		m_contained.translateOrigin(getTranslation()); 
	}
}

void Doom3GroupNode::skinChanged(const std::string& value) {
	if (m_contained.isModel()) {
		// Instantiate a walker class equipped with the new value
		SkinChangedWalker walker(value);
		// Update all children of this node
		traverse(walker);
	}
}

void Doom3GroupNode::refreshModel() {
	// Simulate a "model" key change
	m_contained.modelChanged(_entity.getKeyValue("model"));

	// Trigger a skin change
	skinChanged(_entity.getKeyValue("skin"));
}

void Doom3GroupNode::_onTransformationChanged()
{
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!m_contained.isModel())
	{
		ChildTransformReverter reverter;
		traverse(reverter);

		evaluateTransform();

		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(m_contained.getOrigin());
	}
	else
	{
		// It's a model
		m_contained.revertTransform();
		evaluateTransform();
		m_contained.updateTransform();
	}

	m_contained.m_curveNURBS.curveChanged();
	m_contained.m_curveCatmullRom.curveChanged();
}

void Doom3GroupNode::_applyTransformation()
{
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();

	if (!m_contained.isModel())
	{
		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(m_contained.getOrigin());
	}
}

} // namespace entity
