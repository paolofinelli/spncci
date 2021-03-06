      SUBROUTINE U3MULT(LX1,MX1,LX2,MX2,LX3,MX3,MULTU3,*)               
C     ------------------------------------------------------------------
C     MULTIPLICITY IN U3 COUPLING (SEE MULTTEST FOR VARIOUS VERSIONS)   
C     ... MULTU3 IS FASTER THAN THE DRAAYER ORIGINAL AND MILLENER KAS   
C     ... SUBROUTINE FORM EXECUTES FASTEST ... RETURN 1 BRANCH OPTION   
C     ------------------------------------------------------------------
C     UPDATE/MOD: (LSU,11-78)  J.P.DRAAYER     ORIGINAL FUNCTION FORM   
C                 (BNL,06-87)  J.MILLENER      MILLENER FUNCTION ... KAS
C                 (LSU,10-89)  J.P.DRAAYER     PROFILER OPTIMIZED FORM  
C                 (LSU,11-89)  J.P.DRAAYER     SUBROUTINE FORM FOR SPEED
C     ------------------------------------------------------------------
      MULTU3=0                                                          
      NX=LX1+LX2-LX3-MX1-MX2+MX3                                        
      MX=NX/3                                                           
      IF(3*MX.NE.NX)RETURN 1                                            
      IF(MX.GE.0)THEN                                                   
         L1=LX1                                                         
         L2=LX2                                                         
         L3=LX3                                                         
         M1=MX1                                                         
         M2=MX2                                                         
         M3=MX3                                                         
      ELSE                                                              
         L1=MX1                                                         
         L2=MX2                                                         
         L3=MX3                                                         
         M1=LX1                                                         
         M2=LX2                                                         
         M3=LX3                                                         
         MX=-MX                                                         
      ENDIF                                                             
      NX=MX+M1+M2-M3                                                    
      MU=MIN0(L1-MX,M2)                                                 
      IF(MU.LT.0)RETURN 1                                               
      NU=MIN0(L2-MX,M1)                                                 
      IF(NU.LT.0)RETURN 1                                               
      MULTU3=MAX0(MIN0(NX,NU)-MAX0(NX-MU,0)+1,0)                        
      IF(MULTU3.NE.0)RETURN                                             
      RETURN 1                                                          
      END                                                               
