﻿#include "mruby/wslay.h"
#include "mrb_wslay.h"

#define MRB_WSLAY_ERR mrb_const_get(mrb, mrb_obj_value(mrb_module_get(mrb, "Wslay")), mrb_intern_lit(mrb, "Err"))

typedef struct {
  wslay_event_context_ptr ctx;
  mrb_state *mrb;
  mrb_value handle;
} mrb_wslay_user_data;

static void
mrb_wslay_event_on_msg_recv_callback(wslay_event_context_ptr ctx,
  const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
  mrb_assert(user_data);

  mrb_wslay_user_data *data = (mrb_wslay_user_data *) user_data;

  int ai = mrb_gc_arena_save(data->mrb);
  struct mrb_jmpbuf *prev_jmp = data->mrb->jmp;
  struct mrb_jmpbuf c_jmp;

  MRB_TRY(&c_jmp) {
    data->mrb->jmp = &c_jmp;

    mrb_value argv[4];
    argv[0] = mrb_fixnum_value(arg->rsv);
    argv[1] = mrb_hash_get(data->mrb,
      mrb_const_get(data->mrb,
        mrb_obj_value(mrb_module_get(data->mrb, "Wslay")), mrb_intern_lit(data->mrb, "OpCode")),
      mrb_fixnum_value(arg->opcode));
    argv[2] = mrb_str_new_static(data->mrb, (const char *) arg->msg, arg->msg_length);
    argv[3] = mrb_hash_get(data->mrb,
      mrb_const_get(data->mrb,
        mrb_obj_value(mrb_module_get(data->mrb, "Wslay")), mrb_intern_lit(data->mrb, "StatusCode")),
      mrb_fixnum_value(arg->status_code));

    mrb_yield(data->mrb,
      mrb_iv_get(data->mrb, data->handle,
        mrb_intern_lit(data->mrb, "@on_msg_recv_callback")),
      mrb_obj_new(data->mrb,
        mrb_class_get_under(data->mrb,
          mrb_module_get_under(data->mrb,
            mrb_module_get(data->mrb, "Wslay"), "Event"), "OnMsgRecvArg"), sizeof(argv), argv));

    mrb_gc_arena_restore(data->mrb, ai);

    data->mrb->jmp = prev_jmp;
  } MRB_CATCH(&c_jmp) {
    data->mrb->jmp = prev_jmp;
    wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
    mrb_gc_arena_restore(data->mrb, ai);
    MRB_THROW(data->mrb->jmp);
  } MRB_END_EXC(&c_jmp);
}

static ssize_t
mrb_wslay_event_recv_callback(wslay_event_context_ptr ctx,
  uint8_t *buf, size_t len, int flags, void *user_data)
{
  mrb_assert(user_data);

  mrb_wslay_user_data *data = (mrb_wslay_user_data *) user_data;
  int ai = mrb_gc_arena_save(data->mrb);

  struct mrb_jmpbuf *prev_jmp = data->mrb->jmp;
  struct mrb_jmpbuf c_jmp;

  mrb_int ret = -1;

  MRB_TRY(&c_jmp) {
    data->mrb->jmp = &c_jmp;

    mrb_value buf_obj = mrb_yield(data->mrb,
      mrb_iv_get(data->mrb, data->handle,
        mrb_intern_lit(data->mrb, "@recv_callback")),
      mrb_fixnum_value(len));

    if (mrb_nil_p(buf_obj))
      wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
    else {
      buf_obj = mrb_str_to_str(data->mrb, buf_obj);
      ret = RSTRING_LEN(buf_obj);
      mrb_assert(ret > 0 && ret <= len);
      memcpy(buf, (uint8_t *) RSTRING_PTR(buf_obj), ret);
    }

    data->mrb->jmp = prev_jmp;
  } MRB_CATCH(&c_jmp) {
    data->mrb->jmp = prev_jmp;
    wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
  } MRB_END_EXC(&c_jmp);

  mrb_gc_arena_restore(data->mrb, ai);

  return ret;
}

static ssize_t
mrb_wslay_event_send_callback(wslay_event_context_ptr ctx,
  const uint8_t *buf, size_t len,
  int flags, void *user_data)
{
  mrb_assert(user_data);

  mrb_wslay_user_data *data = (mrb_wslay_user_data *) user_data;
  int ai = mrb_gc_arena_save(data->mrb);

  struct mrb_jmpbuf *prev_jmp = data->mrb->jmp;
  struct mrb_jmpbuf c_jmp;
  mrb_int ret = -1;

  MRB_TRY(&c_jmp) {
    data->mrb->jmp = &c_jmp;

    mrb_value send_ret = mrb_yield(data->mrb,
        mrb_iv_get(data->mrb, data->handle,
          mrb_intern_lit(data->mrb, "@send_callback")),
        mrb_str_new_static(data->mrb, (const char *) buf, len));

    if (mrb_nil_p(send_ret))
      wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
    else {
      ret = mrb_int(data->mrb, send_ret);
      mrb_assert(ret > 0 && ret <= len);
    }

    data->mrb->jmp = prev_jmp;
  } MRB_CATCH(&c_jmp) {
    data->mrb->jmp = prev_jmp;
    wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
  } MRB_END_EXC(&c_jmp);

  mrb_gc_arena_restore(data->mrb, ai);

  return ret;
}

static int
mrb_wslay_event_genmask_callback(wslay_event_context_ptr ctx,
  uint8_t *buf, size_t len,
  void *user_data)
{
  mrb_assert(user_data);

  mrb_wslay_user_data *data = (mrb_wslay_user_data *) user_data;
  int ai = mrb_gc_arena_save(data->mrb);

  struct mrb_jmpbuf *prev_jmp = data->mrb->jmp;
  struct mrb_jmpbuf c_jmp;
  int ret = -1;

  MRB_TRY(&c_jmp) {
    data->mrb->jmp = &c_jmp;

    mrb_value buf_obj = mrb_yield(data->mrb,
      mrb_iv_get(data->mrb, data->handle,
        mrb_intern_lit(data->mrb, "@genmask_callback")),
      mrb_fixnum_value(len));

    buf_obj = mrb_str_to_str(data->mrb, buf_obj);
    mrb_assert(RSTRING_LEN(buf_obj) == len);
    memcpy(buf, (uint8_t *) RSTRING_PTR(buf_obj), RSTRING_LEN(buf_obj));

    ret = 0;

    data->mrb->jmp = prev_jmp;
  } MRB_CATCH(&c_jmp) {
    data->mrb->jmp = prev_jmp;
    wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
  } MRB_END_EXC(&c_jmp);

  mrb_gc_arena_restore(data->mrb, ai);

  return ret;
}

static void
mrb_wslay_user_data_free(mrb_state *mrb, void *p)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) p;
  wslay_event_context_free(data->ctx);
  mrb_free(mrb, p);
}

static const struct mrb_data_type mrb_wslay_user_data_type = {
  "$mrb_i_wslay_user_data", mrb_wslay_user_data_free,
};

static mrb_value
mrb_wslay_event_config_set_no_buffering(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  mrb_bool val;

  mrb_get_args(mrb, "b", &val);

  wslay_event_config_set_no_buffering(data->ctx, val);

  return self;
}

static mrb_value
mrb_wslay_event_config_set_max_recv_msg_length(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  mrb_int val;

  mrb_get_args(mrb, "i", &val);

  if (val < 0||val > UINT64_MAX)
    mrb_raise(mrb, E_RANGE_ERROR, "val is out of range");

  wslay_event_config_set_max_recv_msg_length(data->ctx, val);

  return self;
}

static mrb_value
mrb_wslay_event_recv(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  int err = wslay_event_recv(data->ctx);
  if (err == WSLAY_ERR_NOMEM) {
    mrb->out_of_memory = TRUE;
    mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
  }
  else
  if (err == WSLAY_ERR_CALLBACK_FAILURE)
    mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
  else
  if (err != 0)
    return mrb_hash_get(mrb, MRB_WSLAY_ERR, mrb_fixnum_value(err));

  return self;
}

static mrb_value
mrb_wslay_event_send(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  int err = wslay_event_send(data->ctx);
  if (err == WSLAY_ERR_NOMEM) {
    mrb->out_of_memory = TRUE;
    mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
  }
  else
  if (err == WSLAY_ERR_CALLBACK_FAILURE)
    mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
  else
  if (err != 0)
    return mrb_hash_get(mrb, MRB_WSLAY_ERR, mrb_fixnum_value(err));

  return self;
}

static mrb_value
mrb_wslay_event_queue_msg(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  mrb_int opcode;
  char *msg;
  mrb_int msg_length;

  mrb_get_args(mrb, "is", &opcode, &msg, &msg_length);

  if (opcode < 0||opcode > UINT8_MAX)
    mrb_raise(mrb, E_RANGE_ERROR, "opcode is out of range");

  struct wslay_event_msg msgarg = {
    opcode, (const uint8_t *) msg, msg_length
  };

  int err = wslay_event_queue_msg(data->ctx, &msgarg);
  if (err == WSLAY_ERR_NOMEM) {
    mrb->out_of_memory = TRUE;
    mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
  }
  else
  if (err == WSLAY_ERR_NO_MORE_MSG)
    mrb_raise(mrb, E_WSLAY_ERROR, "further message queueing is not allowed");
  else
  if (err == WSLAY_ERR_INVALID_ARGUMENT)
    mrb_raise(mrb, E_WSLAY_ERROR, "the given message is invalid");
  else
  if (err != 0)
    return mrb_hash_get(mrb, MRB_WSLAY_ERR, mrb_fixnum_value(err));

  return self;
}

static mrb_value
mrb_wslay_event_want_read(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_bool_value(wslay_event_want_read(data->ctx));
}

static mrb_value
mrb_wslay_event_want_write(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_bool_value(wslay_event_want_write(data->ctx));
}

static mrb_value
mrb_wslay_event_get_close_received(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_bool_value(wslay_event_get_close_received(data->ctx));
}

static mrb_value
mrb_wslay_event_get_close_sent(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_bool_value(wslay_event_get_close_sent(data->ctx));
}

static mrb_value
mrb_wslay_event_get_status_code_received(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_fixnum_value(wslay_event_get_status_code_received(data->ctx));
}

static mrb_value
mrb_wslay_event_get_status_code_sent(mrb_state *mrb, mrb_value self)
{
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) DATA_PTR(self);
  mrb_assert(data);

  return mrb_fixnum_value(wslay_event_get_status_code_sent(data->ctx));
}

static mrb_value
mrb_wslay_event_context_server_init(mrb_state *mrb, mrb_value self)
{
  mrb_value callbacks_obj;

  mrb_get_args(mrb, "o", &callbacks_obj);

  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@recv_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "recv_callback missing");
  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@send_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "send_callback missing");
  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@on_msg_recv_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "on_msg_recv_callback missing");
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "callbacks"), callbacks_obj);
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) mrb_calloc(mrb, 1, sizeof(mrb_wslay_user_data));
  data->mrb = mrb;
  data->handle = callbacks_obj;
  mrb_data_init(self, data, &mrb_wslay_user_data_type);
  static struct wslay_event_callbacks server_callbacks = {
    mrb_wslay_event_recv_callback,
    mrb_wslay_event_send_callback,
    NULL,
    NULL,
    NULL,
    NULL,
    mrb_wslay_event_on_msg_recv_callback
  };

  int err = wslay_event_context_server_init(&data->ctx, &server_callbacks, data);
  if (err == WSLAY_ERR_NOMEM) {
    mrb->out_of_memory = TRUE;
    mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
  }

  return self;
}

static mrb_value
mrb_wslay_event_context_client_init(mrb_state *mrb, mrb_value self)
{
  mrb_value callbacks_obj;

  mrb_get_args(mrb, "o", &callbacks_obj);

  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@recv_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "recv_callback missing");
  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@send_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "send_callback missing");
  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@genmask_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "genmask_callback missing");
  if (mrb_type(mrb_iv_get(mrb, callbacks_obj, mrb_intern_lit(mrb, "@on_msg_recv_callback"))) != MRB_TT_PROC)
    mrb_raise(mrb, E_ARGUMENT_ERROR, "on_msg_recv_callback missing");
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "callbacks"), callbacks_obj);
  mrb_wslay_user_data *data = (mrb_wslay_user_data *) mrb_calloc(mrb, 1, sizeof(mrb_wslay_user_data));
  data->mrb = mrb;
  data->handle = callbacks_obj;
  mrb_data_init(self, data, &mrb_wslay_user_data_type);
  static struct wslay_event_callbacks client_callbacks = {
    mrb_wslay_event_recv_callback,
    mrb_wslay_event_send_callback,
    mrb_wslay_event_genmask_callback,
    NULL,
    NULL,
    NULL,
    mrb_wslay_event_on_msg_recv_callback
  };

  int err = wslay_event_context_client_init(&data->ctx, &client_callbacks, data);
  if (err == WSLAY_ERR_NOMEM) {
    mrb->out_of_memory = TRUE;
    mrb_exc_raise(mrb, mrb_obj_value(mrb->nomem_err));
  }

  return self;
}

void
mrb_mruby_wslay_gem_init(mrb_state* mrb) {
  struct RClass *wslay_mod, *wslay_error_cl, *wslay_event_mod,
  *wslay_event_context_cl, *wslay_event_context_server_cl, *wslay_event_context_client_cl;

  wslay_mod = mrb_define_module(mrb, "Wslay");
  wslay_error_cl = mrb_define_class_under(mrb, wslay_mod, "Error", E_RUNTIME_ERROR);
  mrb_value wslay_error_hash = mrb_hash_new_capa(mrb, 9 * 2);
  mrb_define_const(mrb, wslay_mod, "Err", wslay_error_hash);
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_WANT_READ), mrb_symbol_value(mrb_intern_lit(mrb, "want_read")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_WANT_WRITE), mrb_symbol_value(mrb_intern_lit(mrb, "want_write")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_PROTO), mrb_symbol_value(mrb_intern_lit(mrb, "proto")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_INVALID_ARGUMENT), mrb_symbol_value(mrb_intern_lit(mrb, "invalid_argument")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_INVALID_CALLBACK), mrb_symbol_value(mrb_intern_lit(mrb, "invalid_callback")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_NO_MORE_MSG), mrb_symbol_value(mrb_intern_lit(mrb, "no_more_msg")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_CALLBACK_FAILURE), mrb_symbol_value(mrb_intern_lit(mrb, "callback_failure")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_WOULDBLOCK), mrb_symbol_value(mrb_intern_lit(mrb, "wouldblock")));
  mrb_hash_set(mrb, wslay_error_hash, mrb_fixnum_value(WSLAY_ERR_NOMEM), mrb_symbol_value(mrb_intern_lit(mrb, "nomem")));
  int ai = mrb_gc_arena_save(mrb);
  mrb_value wslay_error_hash_keys = mrb_hash_keys(mrb, wslay_error_hash);
  for (mrb_int i = 0; i < RARRAY_LEN(wslay_error_hash_keys); i++) {
    mrb_value key = mrb_ary_ref(mrb, wslay_error_hash_keys, i);
    mrb_hash_set(mrb, wslay_error_hash,
      mrb_hash_get(mrb, wslay_error_hash, key), key);
  }
  mrb_gc_arena_restore(mrb, ai);

  mrb_value wslay_status_code_hash = mrb_hash_new_capa(mrb, 12 * 2);
  mrb_define_const(mrb, wslay_mod, "StatusCode", wslay_status_code_hash);
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_NORMAL_CLOSURE), mrb_symbol_value(mrb_intern_lit(mrb, "normal_closure")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_GOING_AWAY), mrb_symbol_value(mrb_intern_lit(mrb, "going_away")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_PROTOCOL_ERROR), mrb_symbol_value(mrb_intern_lit(mrb, "protocol_error")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_UNSUPPORTED_DATA), mrb_symbol_value(mrb_intern_lit(mrb, "unsupported_data")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_NO_STATUS_RCVD), mrb_symbol_value(mrb_intern_lit(mrb, "no_status_rcvd")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_ABNORMAL_CLOSURE), mrb_symbol_value(mrb_intern_lit(mrb, "abnormal_closure")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_INVALID_FRAME_PAYLOAD_DATA), mrb_symbol_value(mrb_intern_lit(mrb, "invalid_frame_payload_data")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_POLICY_VIOLATION), mrb_symbol_value(mrb_intern_lit(mrb, "policy_violation")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_MESSAGE_TOO_BIG), mrb_symbol_value(mrb_intern_lit(mrb, "message_too_big")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_MANDATORY_EXT), mrb_symbol_value(mrb_intern_lit(mrb, "mandatory_ext")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_INTERNAL_SERVER_ERROR), mrb_symbol_value(mrb_intern_lit(mrb, "internal_server_error")));
  mrb_hash_set(mrb, wslay_status_code_hash, mrb_fixnum_value(WSLAY_CODE_TLS_HANDSHAKE), mrb_symbol_value(mrb_intern_lit(mrb, "tls_handshake")));
  ai = mrb_gc_arena_save(mrb);
  mrb_value wslay_status_code_hash_keys = mrb_hash_keys(mrb, wslay_status_code_hash);
  for (mrb_int i = 0; i < RARRAY_LEN(wslay_status_code_hash_keys); i++) {
    mrb_value key = mrb_ary_ref(mrb, wslay_status_code_hash_keys, i);
    mrb_hash_set(mrb, wslay_status_code_hash,
      mrb_hash_get(mrb, wslay_status_code_hash, key), key);
  }
  mrb_gc_arena_restore(mrb, ai);

  mrb_value io_flags_hash = mrb_hash_new_capa(mrb, 2);
  mrb_define_const(mrb, wslay_mod, "IoFlags", io_flags_hash);
  mrb_hash_set(mrb, io_flags_hash, mrb_fixnum_value(WSLAY_MSG_MORE), mrb_symbol_value(mrb_intern_lit(mrb, "msg_more")));
  mrb_hash_set(mrb, io_flags_hash, mrb_symbol_value(mrb_intern_lit(mrb, "msg_more")), mrb_fixnum_value(WSLAY_MSG_MORE));

  mrb_value wslay_opcode_hash = mrb_hash_new_capa(mrb, 6 * 2);
  mrb_define_const(mrb, wslay_mod, "Opcode", wslay_opcode_hash);
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_CONTINUATION_FRAME), mrb_symbol_value(mrb_intern_lit(mrb, "continuation_frame")));
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_TEXT_FRAME), mrb_symbol_value(mrb_intern_lit(mrb, "text_frame")));
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_BINARY_FRAME), mrb_symbol_value(mrb_intern_lit(mrb, "binary_frame")));
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_CONNECTION_CLOSE), mrb_symbol_value(mrb_intern_lit(mrb, "connection_close")));
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_PING), mrb_symbol_value(mrb_intern_lit(mrb, "ping")));
  mrb_hash_set(mrb, wslay_opcode_hash, mrb_fixnum_value(WSLAY_PONG), mrb_symbol_value(mrb_intern_lit(mrb, "pong")));
  ai = mrb_gc_arena_save(mrb);
  mrb_value wslay_opcode_hash_keys = mrb_hash_keys(mrb, wslay_opcode_hash);
  for (mrb_int i = 0; i < RARRAY_LEN(wslay_opcode_hash_keys); i++) {
    mrb_value key = mrb_ary_ref(mrb, wslay_opcode_hash_keys, i);
    mrb_hash_set(mrb, wslay_opcode_hash,
      mrb_hash_get(mrb, wslay_opcode_hash, key), key);
  }
  mrb_gc_arena_restore(mrb, ai);

  wslay_event_mod = mrb_define_module_under(mrb, wslay_mod, "Event");
  wslay_event_context_cl = mrb_define_class_under(mrb, wslay_event_mod, "Context", mrb->object_class);
  MRB_SET_INSTANCE_TT(wslay_event_context_cl, MRB_TT_DATA);
  mrb_define_method(mrb, wslay_event_context_cl, "no_buffering=",         mrb_wslay_event_config_set_no_buffering,        MRB_ARGS_REQ(1));
  mrb_define_method(mrb, wslay_event_context_cl, "max_recv_msg_length=",  mrb_wslay_event_config_set_max_recv_msg_length, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, wslay_event_context_cl, "recv",                  mrb_wslay_event_recv,                           MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "send",                  mrb_wslay_event_send,                           MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "queue_msg",             mrb_wslay_event_queue_msg,                      MRB_ARGS_REQ(2));
  mrb_define_method(mrb, wslay_event_context_cl, "want_read?",            mrb_wslay_event_want_read,                      MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "want_write?",           mrb_wslay_event_want_write,                     MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "close_received?",       mrb_wslay_event_get_close_received,             MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "close_sent?",           mrb_wslay_event_get_close_sent,                 MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "status_code_received",  mrb_wslay_event_get_status_code_received,       MRB_ARGS_NONE());
  mrb_define_method(mrb, wslay_event_context_cl, "status_code_sent",      mrb_wslay_event_get_status_code_sent,           MRB_ARGS_NONE());

  wslay_event_context_server_cl = mrb_define_class_under(mrb, wslay_event_context_cl, "Server", wslay_event_context_cl);
  mrb_define_method(mrb, wslay_event_context_server_cl, "initialize", mrb_wslay_event_context_server_init, MRB_ARGS_REQ(1));

  wslay_event_context_client_cl = mrb_define_class_under(mrb, wslay_event_context_cl, "Client", wslay_event_context_cl);
  mrb_define_method(mrb, wslay_event_context_client_cl, "initialize", mrb_wslay_event_context_client_init, MRB_ARGS_REQ(1));
}

void
mrb_mruby_wslay_gem_final(mrb_state* mrb) {
  /* finalizer */
}