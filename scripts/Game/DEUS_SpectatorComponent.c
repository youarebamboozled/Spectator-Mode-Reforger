[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class DEUS_SpectatorComponentClass extends ScriptComponentClass { }

class DEUS_SpectatorComponent extends ScriptComponent {
	SCR_ChimeraCharacter OGEntity;
	CameraBase OGCamera;
	
	Animation SpecAnim;
	TNodeId SpecCamIdx;
	
	InputManager Input;
	
	ref ScriptInvoker OnDamageStateChangedSpec;
	
	protected bool IsSpectating;
	
	//------------------------------------------------------------------------------------------------
	void StartSpectating(IEntity ent) {
		if (!Input) {
			Input = g_Game.GetInputManager();
		}
		
		Input.AddActionListener("MenuOpen", EActionTrigger.DOWN, OnEscapeDown);
		
		SCR_DamageManagerComponent dmgMngr = SCR_DamageManagerComponent.GetDamageManager(ent);
		if (dmgMngr) {
			ScriptedHitZone hz = ScriptedHitZone.Cast(dmgMngr.GetDefaultHitZone());
			if (hz) {
				OnDamageStateChangedSpec = hz.GetOnDamageStateChanged();
				OnDamageStateChangedSpec.Insert(OnSpectatedEntityDamageStateChanged);
			}
		}
		
		OGEntity = SCR_ChimeraCharacter.Cast(GetOwner());
		OGCamera = g_ARGame.GetCameraManager().CurrentCamera();
		
		SetControlledEntity(ent);
		
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
		
		IsSpectating = true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice) {
		if (OGCamera) {
			
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetControlledEntity(IEntity ent) {
		Rpc(RpcAsk_SetControlledEntity, Replication.FindId(ent), Replication.FindId(g_ARGame.GetPlayerController()));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SetControlledEntity(RplId entId, RplId pcId) {
		IEntity ent = IEntity.Cast(Replication.FindItem(entId));
		PlayerController pc = PlayerController.Cast(Replication.FindItem(pcId));
		if (ent && pc) {
			pc.SetControlledEntity(ent);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void OnSpectatedEntityDamageStateChanged(HitZone hz) {
		if (hz.GetDamageState() == EDamageState.DESTROYED) {
			StopSpectating();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void OnEscapeDown() {
		StopSpectating();
	}
	
	//------------------------------------------------------------------------------------------------
	void StopSpectating() {
		if (!Input) {
			Input = g_Game.GetInputManager();
		}
		
		Input.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OnEscapeDown);
		
		OnDamageStateChangedSpec.Remove(OnSpectatedEntityDamageStateChanged);
		
		if (OGEntity.GetDamageManager().GetState() != EDamageState.DESTROYED) {
			SetControlledEntity(OGEntity);
		} else {
			TrySpawnPlayer(g_ARGame.GetPlayerController() ,SCR_SpawnPoint.GetSpawnPoints().GetRandomElement());
		}
		
		IsSpectating = false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void TrySpawnPlayer(notnull PlayerController playerController, notnull SCR_SpawnPoint spawnPoint){
		SCR_RespawnComponent spawnComponent = SCR_RespawnComponent.Cast(playerController.FindComponent(SCR_RespawnComponent));
		SCR_PlayerLoadoutComponent loadoutComponent = SCR_PlayerLoadoutComponent.Cast(playerController.FindComponent(SCR_PlayerLoadoutComponent));
		
		SCR_BasePlayerLoadout loadout = loadoutComponent.GetAssignedLoadout();
		ResourceName loadoutResource = loadout.GetLoadoutResource();
		SCR_SpawnData spawnData = new SCR_SpawnPointSpawnData(loadoutResource, spawnPoint.GetRplId());		
		
		spawnComponent.RequestSpawn(spawnData);
	}		 
	
	//------------------------------------------------------------------------------------------------
	bool IsCurrentlySpectating() {
		return IsSpectating;
	}
}
