[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DEUS_SpectateContextAction extends SCR_BaseContextAction {
	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags) {
		ChimeraWorld world = GetGame().GetWorld();
		if (world.IsGameTimePaused()) {
			return false;
		}
		
		if (!hoveredEntity.GetOwner()) {
			return false;
		}
		
		return SCR_ChimeraCharacter.Cast(hoveredEntity.GetOwner());
	}
	
	override void PerformOwner(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition,int flags, int param = -1) {
		IEntity ent = hoveredEntity.GetOwner();
		if (!ent) {
			return;
		}
		
		DEUS_SpectatorComponent comp = DEUS_SpectatorComponent.Cast(g_ARGame.GetPlayerController().GetControlledEntity().FindComponent(DEUS_SpectatorComponent));
		if (!comp) {
			return;
		}
		
		comp.StartSpectating(ent);
	}
}