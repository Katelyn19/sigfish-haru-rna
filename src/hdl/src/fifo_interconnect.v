/*
author: Katelyn Mak
created: 28/02/2024

This module attempts to instantiate a fifo between the AXI S2MM and M2SS.
It will be later extended to implement multi-channel axi dma.
This module is based on the dtw_accel module from the project HARU.

*/
`timescale 1ps / 1ps

module fifo_interconnect #(
	parameter ADDR_WIDTH			= 16,
	parameter DATA_WIDTH			= 32,

	parameter AXIS_DATA_WIDTH			= 32,
	parameter AXIS_KEEP_WIDTH			= (AXIS_DATA_WIDTH / 8),
	parameter AXIS_DATA_USER_WIDTH		= 0,
	parameter FIFO_DATA_WIDTH			= AXIS_DATA_WIDTH,
	parameter FIFO_DEPTH				= 4,
	parameter INVERT_AXI_RESET			= 1,
	parameter INVERT_AXIS_RESET			= 1
)(
	input	wire								S_AXI_clk,
	input	wire								S_AXI_rst,

	// Write Address Channel
	input	wire								S_AXI_awvalid,
	input	wire [ADDR_WIDTH - 1: 0]			S_AXI_awaddr,
	output wire									S_AXI_awready,

	// Write Data Channel
	input	wire								S_AXI_wvalid,
	output wire									S_AXI_wready,
	input	wire [DATA_WIDTH - 1: 0]			S_AXI_wdata,

	// Write Response Channel
	output wire									S_AXI_bvalid,
	input	wire								S_AXI_bready,
	output wire [1:0]							S_AXI_bresp,

	// Read Address Channel
	input	wire								S_AXI_arvalid,
	output wire									S_AXI_arready,
	input	wire [ADDR_WIDTH - 1: 0]			S_AXI_araddr,

	// Read Data Channel
	output wire									S_AXI_rvalid,
	input	wire								S_AXI_rready,
	output wire [1:0]							S_AXI_rresp,
	output wire [DATA_WIDTH - 1: 0]				S_AXI_rdata,

	// AXI Stream

	// Input AXI Stream
	input	wire								SRC_AXIS_clk,
	input	wire								SRC_AXIS_rst,
	input	wire								SRC_AXIS_tuser,
	input	wire								SRC_AXIS_tvalid,
	output wire									SRC_AXIS_tready,
	input	wire								SRC_AXIS_tlast,
	input	wire [AXIS_DATA_WIDTH - 1:0]		SRC_AXIS_tdata,

	// Output AXI Stream
	input	wire								SINK_AXIS_clk,
	input	wire								SINK_AXIS_rst,
	output wire									SINK_AXIS_tuser,
	output wire									SINK_AXIS_tvalid,
	input	wire								SINK_AXIS_tready,
	output wire									SINK_AXIS_tlast,
	output wire [AXIS_DATA_WIDTH - 1:0]			SINK_AXIS_tdata
);

/* ===============================
 * local parameters
 * =============================== */
localparam				REG_CONTROL				= 0;
localparam	integer		ADDR_LSB				= (DATA_WIDTH / 32) + 1;
localparam	integer		ADDR_BITS				= 3;

localparam REFMEM_PTR_WIDTH = 18;

/* ===============================
 * registers/wires
 * =============================== */
// User Interface
wire							w_axi_rst;
wire							w_axis_rst;
wire	[ADDR_WIDTH - 1 : 0]	w_reg_address;
reg								r_reg_invalid_addr;

wire							w_reg_in_rdy;
reg								r_reg_in_ack_stb;
wire	[DATA_WIDTH - 1 : 0]	w_reg_in_data;

wire							w_reg_out_req;
reg								r_reg_out_rdy_stb;
reg		[DATA_WIDTH - 1 : 0]	r_reg_out_data;

// DTW accel
reg		[DATA_WIDTH - 1 : 0]	r_control;
wire	[DATA_WIDTH - 1 : 0]	w_status;
reg		[DATA_WIDTH - 1 : 0]	r_ref_len;
wire	[DATA_WIDTH - 1 : 0]	w_version;
wire	[DATA_WIDTH - 1 : 0]	w_key;
reg		[DATA_WIDTH - 1 : 0]	r_dbg_ref_addr;
reg		[DATA_WIDTH - 1 : 0]	r_dbg_ref_din;
wire	[DATA_WIDTH - 1 : 0]	w_dbg_ref_dout;
wire	[DATA_WIDTH - 1 : 0]	w_dtw_core_cycle_counter;

// Control Register bits
wire							w_dtw_core_rst;
wire							w_dtw_core_rs;
wire							w_dtw_core_mode;

// Status Register bits
wire							w_dtw_core_busy;
wire							w_dtw_core_load_done;

// Src FIFO
wire							w_src_fifo_clear;
wire	[FIFO_DATA_WIDTH - 1:0]	w_src_fifo_w_data;
wire							w_src_fifo_w_stb;
wire							w_src_fifo_full;
wire							w_src_fifo_not_full;

wire	[FIFO_DATA_WIDTH - 1:0]	w_src_fifo_r_data;
wire							w_src_fifo_r_stb;
wire							w_src_fifo_empty;
wire							w_src_fifo_not_empty;

// Sink FIFO
wire	[FIFO_DATA_WIDTH - 1:0]	w_sink_fifo_w_data;
wire							w_sink_fifo_w_stb;
wire							w_sink_fifo_full;
wire							w_sink_fifo_not_full;
wire							w_sink_fifo_r_last;

wire	[FIFO_DATA_WIDTH - 1:0]	w_sink_fifo_r_data;
wire							w_sink_fifo_r_stb;
wire							w_sink_fifo_empty;
wire							w_sink_fifo_not_empty;

// dtw core debug
wire	[2:0]					w_dtw_core_state;
wire	[REFMEM_PTR_WIDTH-1:0]	w_dtw_core_addr_ref;
wire	[31:0]					w_dtw_core_nquery;
wire	[31:0]					w_dtw_core_curr_qid;


/* ===============================
 * initialization
 * =============================== */
initial begin
	r_control <= 0;
	r_ref_len <= 4000;
end

/* ===============================
 * submodules
 * =============================== */
// Convert AXI Slave bus to a simple register/address strobe
axi_lite_slave #(
	.ADDR_WIDTH			(ADDR_WIDTH),
	.DATA_WIDTH			(DATA_WIDTH)
) axi_lite_reg_interface (
	.clk				(S_AXI_clk),
	.rst				(w_axi_rst),

	.i_awvalid			(S_AXI_awvalid),
	.i_awaddr				(S_AXI_awaddr),
	.o_awready			(S_AXI_awready),

	.i_wvalid				(S_AXI_wvalid),
	.o_wready				(S_AXI_wready),
	.i_wdata			(S_AXI_wdata),

	.o_bvalid				(S_AXI_bvalid),
	.i_bready				(S_AXI_bready),
	.o_bresp			(S_AXI_bresp),

	.i_arvalid			(S_AXI_arvalid),
	.o_arready			(S_AXI_arready),
	.i_araddr				(S_AXI_araddr),

	.o_rvalid				(S_AXI_rvalid),
	.i_rready				(S_AXI_rready),
	.o_rresp			(S_AXI_rresp),
	.o_rdata			(S_AXI_rdata),


	// Register Interface
	.o_reg_address		(w_reg_address),
	.i_reg_invalid_addr (r_reg_invalid_addr),

	// From Master
	.o_reg_in_rdy			(w_reg_in_rdy),
	.i_reg_in_ack_stb		(r_reg_in_ack_stb),
	.o_reg_in_data		(w_reg_in_data),

	// To Master
	.o_reg_out_req		(w_reg_out_req),
	.i_reg_out_rdy_stb	(r_reg_out_rdy_stb),
	.i_reg_out_data		(r_reg_out_data)
);


// AXIS src -> src FIFO
axis_2_fifo_adapter #(
	.AXIS_DATA_WIDTH	(AXIS_DATA_WIDTH)
) a2fa (
	.i_axis_tuser		(SRC_AXIS_tuser),
	.i_axis_tvalid		(SRC_AXIS_tvalid),
	.o_axis_tready		(SRC_AXIS_tready),
	.i_axis_tlast		(SRC_AXIS_tlast),
	.i_axis_tdata		(SRC_AXIS_tdata),

	.o_fifo_data		(w_src_fifo_w_data),
	.o_fifo_w_stb		(w_src_fifo_w_stb),
	.i_fifo_not_full	(w_src_fifo_not_full)
);

// Only 1 FIFO is connected to both the axis_2_fifo_adapter and fifo_2_axis_adapter
fifo #(
	.DEPTH				(FIFO_DEPTH),
	.WIDTH				(FIFO_DATA_WIDTH)
) src_and_sink_fifo (
	.clk				(SRC_AXIS_clk),
	.rst				(w_axis_rst | w_src_fifo_clear),

	.i_fifo_w_stb		(w_src_fifo_w_stb),
	.i_fifo_w_data		(w_src_fifo_w_data),
	.o_fifo_full		(w_src_fifo_full),
	.o_fifo_not_full	(w_src_fifo_not_full),

	.i_fifo_r_stb		(w_src_fifo_r_stb),
	.o_fifo_r_data		(w_src_fifo_r_data),
	.o_fifo_empty		(w_src_fifo_empty),
	.o_fifo_not_empty	(w_src_fifo_not_empty)
);
//
// sink FIFO -> AXIS sink
fifo_2_axis_adapter #(
	.AXIS_DATA_WIDTH	(AXIS_DATA_WIDTH)
)f2aa(
	.o_fifo_r_stb		(w_src_fifo_r_stb),
	.i_fifo_data		(w_src_fifo_r_data),
	.i_fifo_not_empty	(w_src_fifo_not_empty),
	.i_fifo_last		(w_src_fifo_empty),

	.o_axis_tuser		(SINK_AXIS_tuser),
	.o_axis_tdata		(SINK_AXIS_tdata),
	.o_axis_tvalid		(SINK_AXIS_tvalid),
	.i_axis_tready		(SINK_AXIS_tready),
	.o_axis_tlast		(SINK_AXIS_tlast)
);

/* ===============================
 * asynchronous logic
 * =============================== */
assign w_axi_rst						= INVERT_AXI_RESET	? ~S_AXI_rst	: S_AXI_rst;
assign w_axis_rst						= INVERT_AXIS_RESET ? ~SRC_AXIS_rst : SRC_AXIS_rst;
assign w_key							= 32'h0ca7cafe;
assign w_src_fifo_clear					= r_control[0];

/* ===============================
 * synchronous logic
 * =============================== */
always @ (posedge S_AXI_clk) begin
	// De-assert Strobes
	r_reg_in_ack_stb	<=	0;
	r_reg_out_rdy_stb		<=  0;
	r_reg_invalid_addr	<=  0;

	if (w_axi_rst) begin
		r_reg_out_data	<=  0;

		// Reset registers
		r_control			<=  0;
		r_ref_len			<=  0;
	end else begin
		if (w_reg_in_rdy) begin
			// M_AXI to here
			case (w_reg_address[ADDR_LSB + ADDR_BITS:ADDR_LSB])
			REG_CONTROL: begin
				r_control <= w_reg_in_data;
			end
			default: begin // unknown address
				$display ("Unknown address: 0x%h", w_reg_address);
				r_reg_invalid_addr <= 1;
			end
			endcase
			r_reg_in_ack_stb <= 1; // Tell AXI Slave we are done with the data
		end else if (w_reg_out_req) begin
			// Here to M_AXI
			case (w_reg_address[ADDR_LSB + ADDR_BITS:ADDR_LSB])
			REG_CONTROL: begin
				r_reg_out_data <= r_control;
			end
			default: begin // Unknown address
				r_reg_out_data		<= 32'h00;
				r_reg_invalid_addr	<= 1;
			end
			endcase
			r_reg_out_rdy_stb <= 1; // Tell AXI Slave to send back this packet
		end
	end
end

endmodule