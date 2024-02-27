[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class DEUS_SpectatorComponentClass extends ScriptComponentClass { }

class DEUS_SpectatorComponent extends ScriptComponent {
	private     CameraBase                             	OGCamera;
	private     CharacterIdentityComponent             	IdentityComponent;
	private     EquipedLoadoutStorageComponent         	ELSComponent;
	private     IEntity                                	HeadCoverEntity;
	private     SCR_CharacterCameraHandlerComponent    	CharCameraHandlerComponent;
	private		EventHandlerManagerComponent 			EventHandler;
	// private		CharacterControllerComponent			CharacterController;
	
	private     InputManager                           	Input;
	
	private ref ScriptInvoker                          	OnDamageStateChangedSpec;
	
	protected   bool                                   	IsSpectating;
	protected   bool                                   	IsBeingSpectated;
	private     bool                                   	IsSpectatedInFirstPerson;
	
	private     vector                                 	CurrentCameraTransform[4];
	private     vector                                 	TargetCameraTransform[4];
	
	private     const float                            	LerpFactor = 0.05;
	private     float                                  	TargetVerticalFOV;
	private     float                                  	CurrentVerticalFOV;
	
	private		RplId                                  	SpectatorId;	
	
	//------------------------------------------------------------------------------------------------
	void ~DEUS_SpectatorComponent() {
		OGCamera = null;
		SpectatedCamera = null;
		IdentityComponent = null;
		ELSComponent = null;
		HeadCoverEntity = null;
		CharCameraHandlerComponent = null;
		Input = null;
		OnDamageStateChangedSpec = null;
	}
	
	//------------------------------------------------------------------------------------------------
    // Replication Methods
    //------------------------------------------------------------------------------------------------
 	
	//------------------------------------------------------------------------------------------------
	private void SetBeingSpectated(IEntity ent, bool state) {
		Rpc(RpcAsk_SetBeingSpectated, Replication.FindId(ent), state);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void RpcAsk_SetBeingSpectated(RplId entId, bool state) {
		IEntity ent = IEntity.Cast(Replication.FindItem(entId));
		DEUS_SpectatorComponent comp = DEUS_SpectatorComponent.Cast(ent.FindComponent(DEUS_SpectatorComponent));
		
		comp.SetBeingSpectatedOwner(Replication.FindId(GetOwner()), state);
	}
	
	//------------------------------------------------------------------------------------------------
	private void SetBeingSpectatedOwner(RplId spectatorId, bool state) {
		Rpc(RpcDo_SetBeingSpectated, spectatorId, state);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	private void RpcDo_SetBeingSpectated(RplId spectatorId, bool state) {
		IEntity owner = GetOwner();
		
		if (state) {
			IsBeingSpectated = state;
			SpectatorId = spectatorId;
			
			CharCameraHandlerComponent = SCR_CharacterCameraHandlerComponent.Cast(owner.FindComponent(SCR_CharacterCameraHandlerComponent));
			CharCameraHandlerComponent.GetThirdPersonSwitchInvoker().Insert(OnThirdPersonSwitch);
			
			EventHandler = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
			EventHandler.RegisterScriptHandler("OnADSChanged", this, OnADSChanged);
			
			Rpc(RpcAsk_OnThirdPersonSwitch, SpectatorId, CharCameraHandlerComponent.IsInThirdPerson());
			
			SetEventMask(owner, EntityEvent.POSTFIXEDFRAME);
		} else {
			IsBeingSpectated = false;
			SpectatorId = -1;
			
			CharCameraHandlerComponent.GetThirdPersonSwitchInvoker().Remove(OnThirdPersonSwitch);
			
			ClearEventMask(owner, EntityEvent.POSTFIXEDFRAME);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void OnADSChanged(BaseWeaponComponent weapon, bool inADS) {
		Rpc(RpcAsk_OnADSChanged, SpectatorId, inADS);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_OnADSChanged(RplId spectatorId, bool inADS) {
		IEntity spectator = IEntity.Cast(Replication.FindItem(spectatorId));
		
		DEUS_SpectatorComponent comp = DEUS_SpectatorComponent.Cast(spectator.FindComponent(DEUS_SpectatorComponent));
		
		comp.OnADSChangedOwner(inADS);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnADSChangedOwner(bool inADS) {
		Rpc(RpcDo_OnADSChanged, inADS);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_OnADSChanged(bool inADS) {
		if (!IsSpectatedInFirstPerson) {
			HideHeadAndAccessories(inADS);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	private void OnThirdPersonSwitch() {
    	bool isInThirdPerson = CharCameraHandlerComponent.IsInThirdPerson();
		Rpc(RpcAsk_OnThirdPersonSwitch, SpectatorId, isInThirdPerson);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void RpcAsk_OnThirdPersonSwitch(RplId spectatorId, bool isInThirdPerson) {
		IEntity spectator = IEntity.Cast(Replication.FindItem(spectatorId));
		
		DEUS_SpectatorComponent comp = DEUS_SpectatorComponent.Cast(spectator.FindComponent(DEUS_SpectatorComponent));
		comp.OnThirdPersonSwitchOwner(isInThirdPerson);
	}
	
	//------------------------------------------------------------------------------------------------
	private void OnThirdPersonSwitchOwner(bool isInThirdPerson) {
		Rpc(RpcDo_OnThirdPersonSwitch, isInThirdPerson);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	private void RpcDo_OnThirdPersonSwitch(bool isInThirdPerson) {
	    IsSpectatedInFirstPerson = !isInThirdPerson;
	    HideHeadAndAccessories(IsSpectatedInFirstPerson);
	}	
	
	//------------------------------------------------------------------------------------------------
	CameraBase SpectatedCamera;
    private void UpdateSpectatorCamera() {
        if (!IsBeingSpectated) {
			return;
		}

        SpectatedCamera = g_ARGame.GetCameraManager().CurrentCamera();
        if (SpectatedCamera) {
			vector mat[4];
            SpectatedCamera.GetTransform(mat);
			
            Rpc(RpcAsk_ReceiveCameraData, SpectatorId, mat, SpectatedCamera.GetVerticalFOV());
        }
    }
	
	//------------------------------------------------------------------------------------------------
    [RplRpc(RplChannel.Unreliable, RplRcver.Server)]
    private void RpcAsk_ReceiveCameraData(RplId spectatorId, vector mat[4], float vFOV) {
		IEntity spectator = IEntity.Cast(Replication.FindItem(spectatorId));
		DEUS_SpectatorComponent comp = DEUS_SpectatorComponent.Cast(spectator.FindComponent(DEUS_SpectatorComponent));
		
		comp.ReceiveCameraDataOwner(mat, vFOV);
    }
	
	//------------------------------------------------------------------------------------------------
    private void ReceiveCameraDataOwner(vector mat[4], float vFOV) {
		Rpc(RpcDo_ReceiveCameraData, mat, vFOV);
	}
	
	//------------------------------------------------------------------------------------------------
    [RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
    private void RpcDo_ReceiveCameraData(vector mat[4], float vFOV) {
        TargetCameraTransform = mat;
		TargetVerticalFOV = vFOV;
    }    
   	
	//------------------------------------------------------------------------------------------------
    // Event Methods
    //------------------------------------------------------------------------------------------------
 	
	//------------------------------------------------------------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice) {
	    ApplyCameraTransform();;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnPostFixedFrame(IEntity owner, float timeSlice) {
	    UpdateSpectatorCamera();
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice) {
	    ApplyCameraTransform();
	}
	
	//------------------------------------------------------------------------------------------------
	void OnEscapeDown() {
		StopSpectating();
	}
	
	//------------------------------------------------------------------------------------------------
	void OnSpectatedEntityDamageStateChanged(HitZone hz) {
		if (hz.GetDamageState() == EDamageState.DESTROYED) {
			StopSpectating();
		}
	}
	
	//------------------------------------------------------------------------------------------------
    // Other Methods
    //------------------------------------------------------------------------------------------------
	
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
		
		SCR_ChimeraCharacter OGEntity = SCR_ChimeraCharacter.Cast(GetOwner());
		
		/* CharacterController = OGEntity.GetCharacterController();
		if (CharacterController) {
			CharacterController.SetDisableViewControls(true);
			CharacterController.SetDisableWeaponControls(true);
			CharacterController.SetDisableMovementControls(true);
		} */
		
		OGCamera = g_ARGame.GetCameraManager().CurrentCamera();
		
		CurrentVerticalFOV = OGCamera.GetVerticalFOV();
		
		if (OGCamera) {
	        OGCamera.GetTransform(CurrentCameraTransform);
	    }
		
		IdentityComponent = CharacterIdentityComponent.Cast(ent.FindComponent(CharacterIdentityComponent));
		
		ELSComponent = EquipedLoadoutStorageComponent.Cast(ent.FindComponent(EquipedLoadoutStorageComponent));
		if (ELSComponent) {
			HeadCoverEntity = ELSComponent.GetClothFromArea(LoadoutHeadCoverArea);
		}
		
		SetBeingSpectated(ent, true);
				
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME | EntityEvent.FRAME);		
		
		IsSpectating = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void StopSpectating() {
		Input.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, OnEscapeDown);
		
		/* if (CharacterController) {
			CharacterController.SetDisableViewControls(true);
			CharacterController.SetDisableWeaponControls(true);
			CharacterController.SetDisableMovementControls(true);
		} */
		
		HideHeadAndAccessories(false);
		
		OnDamageStateChangedSpec.Remove(OnSpectatedEntityDamageStateChanged);
		
		ClearEventMask(GetOwner(), EntityEvent.POSTFRAME | EntityEvent.FRAME);	
		
		IsSpectating = false;
	}	 
	
	//------------------------------------------------------------------------------------------------
	private void ApplyCameraTransform() {
		if (IsSpectating && OGCamera) {
	       	float adjustedLerpFactor = Math.Clamp(LerpFactor, 0, 1);
	
	        CurrentCameraTransform[3] = vector.Lerp(CurrentCameraTransform[3], TargetCameraTransform[3], adjustedLerpFactor);
			vector angles = vector.Lerp(Math3D.MatrixToAngles(CurrentCameraTransform), Math3D.MatrixToAngles(TargetCameraTransform), adjustedLerpFactor);
			Math3D.AnglesToMatrix(angles, CurrentCameraTransform);
			
	        OGCamera.SetTransform(CurrentCameraTransform);
			
			CurrentVerticalFOV = Math.Lerp(CurrentVerticalFOV, TargetVerticalFOV, adjustedLerpFactor);
			OGCamera.SetVerticalFOV(CurrentVerticalFOV);
	    } else if (!OGCamera) {
	        OGCamera = g_ARGame.GetCameraManager().CurrentCamera();
	    }
	}
	
	//------------------------------------------------------------------------------------------------
	private void HideHeadAndAccessories(bool hide = true) {
		if (hide) {
			if (HeadCoverEntity) {
				HeadCoverEntity.ClearFlags(EntityFlags.VISIBLE);
			}
			if (IdentityComponent) {
				IdentityComponent.SetHeadAlpha(255);
			}
		} else {
			if (HeadCoverEntity) {
				HeadCoverEntity.SetFlags(EntityFlags.VISIBLE);
			}
			if (IdentityComponent) {
				IdentityComponent.SetHeadAlpha(0);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsCurrentlySpectating() {
		return IsSpectating;
	}
}
