// ============================================================================
// softmax_exp_lut.sv - Approximate exp(-d/32) using two small lookup tables
//   Decomposition: exp(-d/32) = exp(-int_part) * exp(-frac_part/32)
//   where d = int_part * 32 + frac_part.
//   Output scaled to uint16 (0 = zero, 65535 = 1.0).
// ============================================================================

module softmax_exp_lut
(
    input  logic [7:0]  diff,
    output logic [15:0] exp_val
);

    logic [2:0]  int_part;
    logic [4:0]  frac_part;
    logic [15:0] exp_int;
    logic [15:0] exp_frac;

    assign int_part  = diff[7:5];
    assign frac_part = diff[4:0];

    // exp(-k) * 65535 for k = 0..7
    always_comb
    begin
        case (int_part)
            3'd0: exp_int = 16'd65535;
            3'd1: exp_int = 16'd24109;
            3'd2: exp_int = 16'd8869;
            3'd3: exp_int = 16'd3263;
            3'd4: exp_int = 16'd1200;
            3'd5: exp_int = 16'd442;
            3'd6: exp_int = 16'd162;
            3'd7: exp_int = 16'd60;
        endcase
    end

    // exp(-f/32) * 65535 for f = 0..31
    always_comb
    begin
        case (frac_part)
            5'd0:  exp_frac = 16'd65535;
            5'd1:  exp_frac = 16'd63519;
            5'd2:  exp_frac = 16'd61564;
            5'd3:  exp_frac = 16'd59670;
            5'd4:  exp_frac = 16'd57834;
            5'd5:  exp_frac = 16'd56055;
            5'd6:  exp_frac = 16'd54330;
            5'd7:  exp_frac = 16'd52659;
            5'd8:  exp_frac = 16'd51039;
            5'd9:  exp_frac = 16'd49468;
            5'd10: exp_frac = 16'd47946;
            5'd11: exp_frac = 16'd46471;
            5'd12: exp_frac = 16'd45042;
            5'd13: exp_frac = 16'd43656;
            5'd14: exp_frac = 16'd42313;
            5'd15: exp_frac = 16'd41011;
            5'd16: exp_frac = 16'd39749;
            5'd17: exp_frac = 16'd38526;
            5'd18: exp_frac = 16'd37341;
            5'd19: exp_frac = 16'd36192;
            5'd20: exp_frac = 16'd35078;
            5'd21: exp_frac = 16'd33999;
            5'd22: exp_frac = 16'd32953;
            5'd23: exp_frac = 16'd31939;
            5'd24: exp_frac = 16'd30957;
            5'd25: exp_frac = 16'd30004;
            5'd26: exp_frac = 16'd29081;
            5'd27: exp_frac = 16'd28186;
            5'd28: exp_frac = 16'd27319;
            5'd29: exp_frac = 16'd26479;
            5'd30: exp_frac = 16'd25664;
            5'd31: exp_frac = 16'd24874;
        endcase
    end

    // Combine: (exp_int * exp_frac) >> 16
    /* verilator lint_off UNUSEDSIGNAL */
    logic [31:0] product;
    /* verilator lint_on UNUSEDSIGNAL */
    assign product = {16'b0, exp_int} * {16'b0, exp_frac};
    assign exp_val = product[31:16];

endmodule : softmax_exp_lut
