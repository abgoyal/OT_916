
#ifndef __rt_trace_defs_asm_h
#define __rt_trace_defs_asm_h


#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

/* Register rw_cfg, scope rt_trace, type rw */
#define reg_rt_trace_rw_cfg___en___lsb 0
#define reg_rt_trace_rw_cfg___en___width 1
#define reg_rt_trace_rw_cfg___en___bit 0
#define reg_rt_trace_rw_cfg___mode___lsb 1
#define reg_rt_trace_rw_cfg___mode___width 1
#define reg_rt_trace_rw_cfg___mode___bit 1
#define reg_rt_trace_rw_cfg___owner___lsb 2
#define reg_rt_trace_rw_cfg___owner___width 1
#define reg_rt_trace_rw_cfg___owner___bit 2
#define reg_rt_trace_rw_cfg___wp___lsb 3
#define reg_rt_trace_rw_cfg___wp___width 1
#define reg_rt_trace_rw_cfg___wp___bit 3
#define reg_rt_trace_rw_cfg___stall___lsb 4
#define reg_rt_trace_rw_cfg___stall___width 1
#define reg_rt_trace_rw_cfg___stall___bit 4
#define reg_rt_trace_rw_cfg___wp_start___lsb 8
#define reg_rt_trace_rw_cfg___wp_start___width 7
#define reg_rt_trace_rw_cfg___wp_stop___lsb 16
#define reg_rt_trace_rw_cfg___wp_stop___width 7
#define reg_rt_trace_rw_cfg_offset 0

/* Register rw_tap_ctrl, scope rt_trace, type rw */
#define reg_rt_trace_rw_tap_ctrl___ack_data___lsb 0
#define reg_rt_trace_rw_tap_ctrl___ack_data___width 1
#define reg_rt_trace_rw_tap_ctrl___ack_data___bit 0
#define reg_rt_trace_rw_tap_ctrl___ack_guru___lsb 1
#define reg_rt_trace_rw_tap_ctrl___ack_guru___width 1
#define reg_rt_trace_rw_tap_ctrl___ack_guru___bit 1
#define reg_rt_trace_rw_tap_ctrl_offset 4

/* Register r_tap_stat, scope rt_trace, type r */
#define reg_rt_trace_r_tap_stat___dav___lsb 0
#define reg_rt_trace_r_tap_stat___dav___width 1
#define reg_rt_trace_r_tap_stat___dav___bit 0
#define reg_rt_trace_r_tap_stat___empty___lsb 1
#define reg_rt_trace_r_tap_stat___empty___width 1
#define reg_rt_trace_r_tap_stat___empty___bit 1
#define reg_rt_trace_r_tap_stat_offset 8

/* Register rw_tap_data, scope rt_trace, type rw */
#define reg_rt_trace_rw_tap_data_offset 12

/* Register rw_tap_hdata, scope rt_trace, type rw */
#define reg_rt_trace_rw_tap_hdata___op___lsb 0
#define reg_rt_trace_rw_tap_hdata___op___width 4
#define reg_rt_trace_rw_tap_hdata___sub_op___lsb 4
#define reg_rt_trace_rw_tap_hdata___sub_op___width 4
#define reg_rt_trace_rw_tap_hdata_offset 16

/* Register r_redir, scope rt_trace, type r */
#define reg_rt_trace_r_redir_offset 20


/* Constants */
#define regk_rt_trace_brk                         0x0000000c
#define regk_rt_trace_dbg                         0x00000003
#define regk_rt_trace_dbgdi                       0x00000004
#define regk_rt_trace_dbgdo                       0x00000005
#define regk_rt_trace_gmode                       0x00000000
#define regk_rt_trace_no                          0x00000000
#define regk_rt_trace_nop                         0x00000000
#define regk_rt_trace_normal                      0x00000000
#define regk_rt_trace_rdmem                       0x00000007
#define regk_rt_trace_rdmemb                      0x00000009
#define regk_rt_trace_rdpreg                      0x00000002
#define regk_rt_trace_rdreg                       0x00000001
#define regk_rt_trace_rdsreg                      0x00000003
#define regk_rt_trace_redir                       0x00000006
#define regk_rt_trace_ret                         0x0000000b
#define regk_rt_trace_rw_cfg_default              0x00000000
#define regk_rt_trace_trcfg                       0x00000001
#define regk_rt_trace_wp                          0x00000001
#define regk_rt_trace_wp0                         0x00000001
#define regk_rt_trace_wp1                         0x00000002
#define regk_rt_trace_wp2                         0x00000004
#define regk_rt_trace_wp3                         0x00000008
#define regk_rt_trace_wp4                         0x00000010
#define regk_rt_trace_wp5                         0x00000020
#define regk_rt_trace_wp6                         0x00000040
#define regk_rt_trace_wrmem                       0x00000008
#define regk_rt_trace_wrmemb                      0x0000000a
#define regk_rt_trace_wrpreg                      0x00000005
#define regk_rt_trace_wrreg                       0x00000004
#define regk_rt_trace_wrsreg                      0x00000006
#define regk_rt_trace_yes                         0x00000001
#endif /* __rt_trace_defs_asm_h */
